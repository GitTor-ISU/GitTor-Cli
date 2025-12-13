#include <chrono>
#include <csignal>
#include <filesystem>  // NOLINT(build/c++17)
#include <fstream>
#include <glib.h>  // NOLINT(build/include_order)
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bdecode.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/entry.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/span.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/write_resume_data.hpp>

extern "C" {
#include "service/service_internals.h"
#include "utils/utils.h"
}

namespace fs = std::filesystem;

namespace {

using clk = std::chrono::steady_clock;

typedef struct torrent_t {
    char torrent_name[GIT_OID_HEXSZ + 1];
    char torrent_path[PATH_MAX + 1];
    char resume_path[PATH_MAX + 1];
    char save_path[PATH_MAX + 1];
    clk::time_point last_save_resume;
    lt::torrent_handle handle;
} torrent_t;

// return the name of a torrent status enum
__attribute__((__unused__)) char const* state(lt::torrent_status::state_t s) {
    switch (s) {
        case lt::torrent_status::checking_files:
            return "checking";
        case lt::torrent_status::downloading_metadata:
            return "retrieving metadata";
        case lt::torrent_status::downloading:
            return "downloading";
        case lt::torrent_status::finished:
            return "finished";
        case lt::torrent_status::seeding:
            return "seeding";
        case lt::torrent_status::checking_resume_data:
            return "checking resume";
        default:
            return "<>";
    }
}

std::vector<char> load_file(char const* filename) {
    std::ifstream ifs(filename, std::ios_base::binary);
    ifs.unsetf(std::ios_base::skipws);
    return {std::istream_iterator<char>(ifs), std::istream_iterator<char>()};
}

std::vector<torrent_t> find_torrents(const char* dir) {
    std::vector<torrent_t> result;

    try {
        for (const fs::__cxx11::directory_entry& entry :
             fs::directory_iterator(dir)) {
            if (entry.is_regular_file() &&
                entry.path().extension() == ".torrent") {
                torrent_t t;

                // torrent_name
                const std::string torrent_name = entry.path().stem().string();
                std::strncpy(t.torrent_name, torrent_name.c_str(),
                             GIT_OID_HEXSZ);
                t.torrent_name[sizeof(t.torrent_name) - 1] = '\0';

                // torrent_path
                const std::string torrent_path = entry.path().string();
                std::strncpy(t.torrent_path, torrent_path.c_str(), PATH_MAX);
                t.torrent_path[sizeof(t.torrent_path) - 1] = '\0';

                // resume path
                fs::path resume = entry.path();
                resume.replace_extension(".resume");
                const std::string resume_path = resume.string();
                std::strncpy(t.resume_path, resume_path.c_str(), PATH_MAX);
                t.resume_path[sizeof(t.resume_path) - 1] = '\0';

                // save path
                const std::string save_path =
                    entry.path().parent_path().string();
                std::strncpy(t.save_path, save_path.c_str(), PATH_MAX);
                t.save_path[sizeof(t.save_path) - 1] = '\0';

                // latest resume time
                t.last_save_resume = clk::now();

                result.push_back(t);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error Finding Torrents: " << e.what() << '\n';
    }

    return result;
}

// set when we're exiting
GCancellable* cancellable{NULL};

void sighandler(int) {
    g_cancellable_cancel(cancellable);
}

}  // anonymous namespace

extern "C" gpointer handle_seeding(gpointer data) {
    seed_thread_data_t* seed_data = static_cast<seed_thread_data_t*>(data);
    cancellable = seed_data->connection_cancellable;
    std::signal(SIGINT, &sighandler);
    const char* dir = gittor_remote_dir();

    // Load the session
    char* ses_path = g_build_filename(dir, ".session", NULL);
    lt::entry::preformatted_type session_params = load_file(ses_path);
    free(ses_path);
    lt::session_params params = session_params.empty()
                                    ? lt::session_params()
                                    : lt::read_session_params(session_params);
    params.settings.set_int(lt::settings_pack::alert_mask,
                            lt::alert_category::error |
                                lt::alert_category::storage |
                                lt::alert_category::status);

    // Set up network
    // TODO(Isaac): Use config port
    const int port = 53385;
    const std::string listen_interfaces = "0.0.0.0:" + std::to_string(port);
    params.settings.set_str(lt::settings_pack::listen_interfaces,
                            listen_interfaces);

    lt::session ses(params);

    // Load the torrents
    std::vector<torrent_t> torrents = find_torrents(dir);
    for (torrent_t& t : torrents) {
        lt::entry::preformatted_type buf = load_file(t.resume_path);
        lt::add_torrent_params atp = lt::load_torrent_file(t.torrent_path);

        if (buf.size()) {
            lt::add_torrent_params resume_atp = lt::read_resume_data(buf);
            if (atp.info_hashes == resume_atp.info_hashes)
                atp = std::move(resume_atp);
        }

        atp.save_path = t.save_path;
        atp.userdata = &t;
        ses.async_add_torrent(atp);
    }

    // Seed the torrents
    while (!g_cancellable_is_cancelled(cancellable)) {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);
        const std::vector<lt::torrent_handle> handles = ses.get_torrents();

        for (lt::alert const* a : alerts) {
            // Torrent added
            if (const lt::add_torrent_alert* at =
                    lt::alert_cast<lt::add_torrent_alert>(a)) {
                torrent_t* data = static_cast<torrent_t*>(at->params.userdata);
                data->handle = at->handle;
            }

            // Torrent finished
            if (const lt::torrent_finished_alert* ft =
                    lt::alert_cast<lt::torrent_finished_alert>(a)) {
                ft->handle.save_resume_data(
                    lt::torrent_handle::only_if_modified |
                    lt::torrent_handle::save_info_dict);
            }

            // Torrent resume save
            if (const lt::save_resume_data_alert* rd =
                    lt::alert_cast<lt::save_resume_data_alert>(a)) {
                torrent_t* data =
                    static_cast<torrent_t*>(rd->handle.userdata());
                std::ofstream of(data->resume_path, std::ios_base::binary);
                of.unsetf(std::ios_base::skipws);
                auto const b = write_resume_data_buf(rd->params);
                of.write(b.data(), static_cast<int>(b.size()));
            }

            // Torrent error
            if (const lt::torrent_error_alert* er =
                    lt::alert_cast<lt::torrent_error_alert>(a)) {
                std::clog << a->message() << '\n';
                er->handle.save_resume_data(
                    lt::torrent_handle::only_if_modified |
                    lt::torrent_handle::save_info_dict);
            }

            // State updated
            if (const lt::state_update_alert* st =
                    lt::alert_cast<lt::state_update_alert>(a)) {
                for (const lt::torrent_status& s : st->status) {
                    torrent_t* t = static_cast<torrent_t*>(s.handle.userdata());

                    std::clog << t->torrent_name << ": " << state(s.state)
                              << ' ' << (s.download_payload_rate / 1000)
                              << " kB/s " << (s.total_done / 1000) << " kB ("
                              << (s.progress_ppm / 10000) << "%) downloaded ("
                              << s.num_peers << " peers)\n";
                    std::cout.flush();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // ask the session to post a state_update_alert, to update our
        // state output for the torrent
        ses.post_torrent_updates();

        // save resume data once every 30 seconds
        for (const lt::torrent_handle& h : handles) {
            torrent_t* data = static_cast<torrent_t*>(h.userdata());
            if (clk::now() - data->last_save_resume >
                std::chrono::seconds(30)) {
                h.save_resume_data(lt::torrent_handle::only_if_modified |
                                   lt::torrent_handle::save_info_dict);
                data->last_save_resume = clk::now();
            }
        }
    }

    return NULL;
}

extern "C" int create_torrent(char path[PATH_MAX]) try {
    lt::file_storage fs;

    // recursively adds files in directories
    char* base = g_path_get_basename(path);
    char* dir = g_path_get_dirname(path);
    char tor[PATH_MAX];
    g_snprintf(tor, sizeof(tor), "%s.torrent", path);

    lt::add_files(fs, path);

    // TODO(Isaac): Use config trackers
    lt::create_torrent t(fs);
    t.add_tracker("https://tracker.moeblog.cn:443/announce");
    t.add_tracker("https://tr.nyacat.pw:443/announce");
    t.add_tracker("https://tr.highstar.shop:443/announce");
    t.add_tracker("https://tracker.gcrenwp.top:443/announce");
    // TODO(Isaac): Set as git user
    // t.set_creator("libtorrent example");

    // reads the files and calculates the hashes
    lt::set_piece_hashes(t, dir);

    std::ofstream out(tor, std::ios_base::binary);
    std::vector<char> buf = t.generate_buf();
    out.write(buf.data(), static_cast<std::streamsize>(buf.size()));

    const lt::add_torrent_params atp = lt::load_torrent_buffer(buf);

    std::cout << make_magnet_uri(atp).c_str() << "\n";

    free(base);
    free(dir);
    return 0;
} catch (std::exception& e) {
    std::cerr << "Error Creating Torrent: " << e.what() << '\n';
    return 1;
}
