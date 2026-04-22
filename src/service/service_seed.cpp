#include <chrono>
#include <csignal>
#include <deque>
#include <errno.h>     // NOLINT(build/include_order)
#include <filesystem>  // NOLINT(build/c++17)
#include <fstream>
#include <git2.h>  // NOLINT(build/include_order)
#include <glib.h>  // NOLINT(build/include_order)
#include <iostream>
#include <iterator>
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
#include "config/config.h"
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
char const* state(lt::torrent_status::state_t s) {
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

std::vector<char> load_file(const char* filename) {
    std::ifstream ifs(filename, std::ios_base::binary);
    ifs.unsetf(std::ios_base::skipws);
    return {std::istream_iterator<char>(ifs), std::istream_iterator<char>()};
}

void load_torrent(torrent_t& t, const fs::directory_entry& file) {
    // torrent_name
    const std::string torrent_name = file.path().stem().string();
    std::strncpy(t.torrent_name, torrent_name.c_str(), GIT_OID_HEXSZ);
    t.torrent_name[sizeof(t.torrent_name) - 1] = '\0';

    // torrent_path
    const std::string torrent_path = file.path().string();
    std::strncpy(t.torrent_path, torrent_path.c_str(), PATH_MAX);
    t.torrent_path[sizeof(t.torrent_path) - 1] = '\0';

    // resume path
    fs::path resume = file.path();
    resume.replace_extension(".resume");
    const std::string resume_path = resume.string();
    std::strncpy(t.resume_path, resume_path.c_str(), PATH_MAX);
    t.resume_path[sizeof(t.resume_path) - 1] = '\0';

    // save path
    const std::string save_path = file.path().parent_path().string();
    std::strncpy(t.save_path, save_path.c_str(), PATH_MAX);
    t.save_path[sizeof(t.save_path) - 1] = '\0';

    // latest resume time
    t.last_save_resume = clk::now();
}

std::vector<torrent_t> find_torrents(const char* dir) {
    std::vector<torrent_t> result;
    try {
        for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() &&
                entry.path().extension() == ".torrent") {
                torrent_t t;
                load_torrent(t, entry);
                result.push_back(t);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[GitTor Service thread=";
        std::cerr << reinterpret_cast<void*>(g_thread_self());
        std::cerr << "] Error Finding Torrents: " << e.what() << '\n';
    }

    return result;
}

// set when we're exiting
GCancellable* cancellable{NULL};

void sighandler(int) {
    g_cancellable_cancel(cancellable);
}

int get_creator(char* buf, size_t size) {
    int error = 0;
    git_config* cfg = nullptr;
    git_config* snapshot = nullptr;
    const char* name = nullptr;
    const char* email = nullptr;
    std::string creator = "GitTor";

    if (git_libgit2_init() < 0) {
        error = -1;
    }

    if (!error) {
        error = git_config_open_default(&cfg);
    }

    if (!error) {
        error = git_config_snapshot(&snapshot, cfg);
        git_config_free(cfg);
    }

    if (!error) {
        if (!git_config_get_string(&name, snapshot, "user.name") &&
            !git_config_get_string(&email, snapshot, "user.email")) {
            creator += " (";
            creator += name;
            creator += " <";
            creator += email;
            creator += ">)";
        }
    }

    g_strlcpy(buf, creator.c_str(), size);

    git_config_free(snapshot);
    git_libgit2_shutdown();
    return error;
}

}  // anonymous namespace

extern "C" gpointer handle_seeding(gpointer data) {
    seed_thread_data_t* seed_data = static_cast<seed_thread_data_t*>(data);
    cancellable = seed_data->connection_cancellable;
    GAsyncQueue* queue = seed_data->queue;
    std::signal(SIGINT, &sighandler);
    const char* dir = gittor_remote_dir();

    // Construct the log file
    gchar* log_path =
        g_build_filename(g_get_user_config_dir(), "gittor", "seeder.log", NULL);
    std::ofstream log_file(log_path);
    g_free(log_path);

    std::streambuf* old_clog_buf = nullptr;
    std::streambuf* old_cerr_buf = nullptr;
    if (log_file.is_open()) {
        old_clog_buf = std::clog.rdbuf(log_file.rdbuf());
        old_cerr_buf = std::cerr.rdbuf(log_file.rdbuf());
    } else {
        std::cerr << "Failed to open log file. Output remaining on stdout.\n";
    }

    // Configure the session
    lt::session_params params = lt::session_params();
    params.settings.set_int(lt::settings_pack::alert_mask,
                            lt::alert_category::error |
                                lt::alert_category::storage |
                                lt::alert_category::status);

    // Set up network
    const config_id_t port_config = {.group = "network", .key = "port"};
    char* port_str = config_get(CONFIG_SCOPE_GLOBAL, &port_config, NULL);
    if (port_str != NULL) {
        const std::string listen_interfaces =
            std::string("0.0.0.0:") + port_str;
        params.settings.set_str(lt::settings_pack::listen_interfaces,
                                listen_interfaces);
        free(port_str);
    }

    // Start the session
    lt::session ses(params);

    // Load the torrents into a deque so addresses remain stable when
    // adding/removing
    std::vector<torrent_t> initial = find_torrents(dir);
    std::deque<torrent_t> torrents;
    for (torrent_t& t0 : initial) {
        torrents.push_back(std::move(t0));
    }
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
                std::cerr << a->message() << '\n';
                std::cerr.flush();
                er->handle.save_resume_data(
                    lt::torrent_handle::only_if_modified |
                    lt::torrent_handle::save_info_dict);
            }

            // State updated
            if (const lt::state_update_alert* st =
                    lt::alert_cast<lt::state_update_alert>(a)) {
                for (const lt::torrent_status& s : st->status) {
                    torrent_t* t = static_cast<torrent_t*>(s.handle.userdata());

                    std::clog << "[GitTor Service thread=";
                    std::clog << reinterpret_cast<void*>(g_thread_self());
                    std::clog << "] Repository " << t->torrent_name << ": "
                              << state(s.state) << ' '
                              << (s.download_payload_rate / 1000) << " kB/s "
                              << (s.total_done / 1000) << " kB ("
                              << (s.progress_ppm / 10000) << "%) downloaded ("
                              << s.num_peers << " peers)\n";
                    std::clog.flush();
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

        // Handle the message queue
        seed_thread_queue_item_t* item;
        while ((item = reinterpret_cast<seed_thread_queue_item_t*>(
                    g_async_queue_try_pop(queue)))) {
            char remote_dir[PATH_MAX];
            switch (item->packet.type) {
                case SEED_START: {
                    // Create the torrent file on disk
                    gittor_remote_path(
                        remote_dir, reinterpret_cast<char*>(item->packet.data));
                    create_torrent(remote_dir);

                    // Load the .torrent and add it to the session using a
                    // stable storage
                    const std::string torrent_path =
                        std::string(remote_dir) + ".torrent";
                    const fs::directory_entry entry(torrent_path);
                    torrent_t t;
                    load_torrent(t, entry);

                    lt::entry::preformatted_type buf = load_file(t.resume_path);
                    lt::add_torrent_params atp =
                        lt::load_torrent_file(t.torrent_path);
                    if (buf.size()) {
                        lt::add_torrent_params resume_atp =
                            lt::read_resume_data(buf);
                        if (atp.info_hashes == resume_atp.info_hashes)
                            atp = std::move(resume_atp);
                    }

                    // store the torrent so its address is stable and use that
                    // for userdata
                    torrents.push_back(std::move(t));
                    torrent_t* stored = &torrents.back();
                    atp.save_path = stored->save_path;
                    atp.userdata = stored;
                    ses.async_add_torrent(atp);
                    item->error_code = 0;
                    break;
                }

                case SEED_STOP: {
                    gittor_remote_path(
                        remote_dir, reinterpret_cast<char*>(item->packet.data));
                    const std::string torrent_path =
                        std::string(remote_dir) + ".torrent";
                    gchar* torrent_file = g_strdup(torrent_path.c_str());
                    remove(torrent_file);
                    g_free(torrent_file);

                    // remove from session and our list (match by path)
                    for (auto it = torrents.begin(); it != torrents.end();
                         it++) {
                        if (torrent_path == std::string(it->torrent_path)) {
                            if (it->handle.is_valid()) {
                                ses.remove_torrent(it->handle);
                            }
                            torrents.erase(it);
                            break;
                        }
                    }

                    item->error_code = 0;
                    break;
                }

                default: {
                    item->error_code = 1;
                    break;
                }
            }

            g_mutex_lock(&item->mutex);
            item->ready = true;
            g_cond_signal(&item->cond);
            g_mutex_unlock(&item->mutex);
        }
    }

    if (old_clog_buf) {
        std::clog.rdbuf(old_clog_buf);
    }
    if (old_cerr_buf) {
        std::cerr.rdbuf(old_cerr_buf);
    }

    return NULL;
}

extern "C" int create_torrent(char path[PATH_MAX]) try {
    lt::file_storage fs;
    char tor[PATH_MAX];
    g_snprintf(tor, sizeof(tor), "%s.torrent", path);

    // recursively adds files in directories
    lt::add_files(fs, path);

    // Get the list of trackers from the config
    lt::create_torrent t(fs);
    config_id_t config_id = {.group = "network", .key = NULL};

    for (int i = 1; i < 100; i++) {
        char key[32];
        g_snprintf(key, sizeof(key), "tracker%d", i);
        config_id.key = key;
        char* tracker = config_get(CONFIG_SCOPE_GLOBAL, &config_id, NULL);

        if (tracker == NULL) {
            break;
        }
        t.add_tracker(tracker);
        free(tracker);
    }

    // Set the creator
    char creator[256];
    get_creator(creator, sizeof(creator));
    t.set_creator(creator);

    // reads the files and calculates the hashes
    char* dir = g_path_get_dirname(path);
    lt::set_piece_hashes(t, dir);
    free(dir);

    std::ofstream out(tor, std::ios_base::binary);
    std::vector<char> buf = t.generate_buf();
    out.write(buf.data(), static_cast<std::streamsize>(buf.size()));

    return 0;
} catch (std::exception& e) {
    std::cerr << "Error Creating Torrent: " << e.what() << '\n';
    return 1;
}

extern "C" int get_magnet_link(const char* torrent_path,
                               char* out_magnet,
                               size_t out_size) try {
    if (!torrent_path || !out_magnet || !out_size) {
        return EINVAL;
    }

    const lt::add_torrent_params atp = lt::load_torrent_file(torrent_path);
    const std::string magnet = lt::make_magnet_uri(atp);

    return g_strlcpy(out_magnet, magnet.c_str(), out_size) != magnet.length();
} catch (std::exception& e) {
    std::cerr << "Error Generating Magnet Link: " << e.what() << '\n';
    return -1;
}
