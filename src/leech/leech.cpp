#include <cctype>
#include <chrono>
#include <csignal>
#include <fstream>
#include <glib.h>  // NOLINT(build/include_order)
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
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
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/write_resume_data.hpp>

extern "C" {
#include "api/torrents.h"
#include "config/config.h"
#include "leech/leech.h"  // IWYU pragma: keep
#include "leech/leech_internal.h"
#include "seed/seed.h"
#include "utils/utils.h"
}

namespace {

using clk = std::chrono::steady_clock;

// return the name of a torrent status enum
char const* state(lt::torrent_status::state_t s) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
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
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}

std::vector<char> load_file(char const* filename) {
    std::ifstream ifs(filename, std::ios_base::binary);
    ifs.unsetf(std::ios_base::skipws);
    return {std::istream_iterator<char>(ifs), std::istream_iterator<char>()};
}

// set when we're exiting
std::atomic<bool> shut_down{false};

void sighandler(int) {
    shut_down = true;
}

std::string sanitize_file_name(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    bool last_was_sep = false;
    for (const unsigned char c : input) {
        if (std::isalnum(c) || c == '.' || c == '-' || c == '_') {
            out.push_back(static_cast<char>(c));
            last_was_sep = false;
        } else if (!last_was_sep && !out.empty()) {
            out.push_back('_');
            last_was_sep = true;
        }
    }

    while (!out.empty() && (out.back() == '_' || out.back() == '.')) {
        out.pop_back();
    }

    if (out.empty()) {
        return "download";
    }

    return out;
}

void write_torrent_file(const std::string& path,
                        const std::shared_ptr<const lt::torrent_info>& ti) {
    if (!ti) {
        return;
    }

    const lt::create_torrent ct(*ti);
    const lt::entry e = ct.generate();
    std::vector<char> data;
    lt::bencode(std::back_inserter(data), e);

    std::ofstream of(path, std::ios_base::binary);
    of.unsetf(std::ios_base::skipws);
    of.write(data.data(), static_cast<int>(data.size()));
}

}  // namespace

extern "C" int leech_repository(const char* key,
                                key_type_e type,
                                char* output_path,
                                size_t output_path_size) try {
    std::string dir = gittor_remote_dir();
    if (!dir.empty() && g_mkdir_with_parents(dir.c_str(), 0755) != 0) {
        throw std::runtime_error("Failed to create remote directory: " + dir);
    }
    if (!dir.empty() && dir.back() != '/') {
        dir.push_back('/');
    }
    std::string torrent_file_path;
    bool should_write_torrent_file = false;

    // Load the torrent
    lt::add_torrent_params atp;
    switch (type) {
        case REPO_ID: {
            api_result_e result = API_OK;
            torrent_dto_t* torrent = api_get_torrent_by_repo_id(key, &result);
            if (!torrent) {
                throw std::runtime_error(
                    "Failed to fetch torrent by repository id");
            }

            torrent_file_path = dir + key + std::string(".torrent");
            if (api_get_torrent_file(torrent->id, torrent_file_path.c_str(),
                                     &result) != 0) {
                torrent_dto_free(torrent);
                throw std::runtime_error("Failed to download torrent file");
            }

            torrent_dto_free(torrent);
            atp = lt::load_torrent_file(torrent_file_path);
            break;
        }
        case MAGNET_LINK:
            atp = lt::parse_magnet_uri(key);
            should_write_torrent_file = true;
            break;
        case TORRENT_PATH:
            atp = lt::load_torrent_file(key);
            should_write_torrent_file = true;
            break;
        default:
            throw std::logic_error("Unknown key type");
    }

    // Get the torrent name
    std::string torrent_name;
    if (atp.ti) {
        torrent_name = atp.ti->name();
    } else {
        torrent_name = atp.name;
    }

    // Sanitize it to name files off of
    torrent_name = sanitize_file_name(torrent_name);

    gittor_seed_stop(torrent_name.c_str());

    if (should_write_torrent_file) {
        torrent_file_path = dir + torrent_name + ".torrent";
        if (atp.ti) {
            write_torrent_file(torrent_file_path, atp.ti);
            should_write_torrent_file = false;
        }
    }

    // load session parameters
    std::vector<char> session_params =
        load_file((dir + torrent_name + ".session").c_str());
    lt::session_params params = session_params.empty()
                                    ? lt::session_params()
                                    : lt::read_session_params(session_params);
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

    lt::session ses(params);
    clk::time_point last_save_resume = clk::now();

    // load resume data from disk and pass it in as we add the magnet link
    std::vector<char> buf = load_file((dir + torrent_name + ".resume").c_str());

    if (buf.size()) {
        lt::add_torrent_params atp_partial = lt::read_resume_data(buf);
        if (atp_partial.info_hashes == atp.info_hashes)
            atp = std::move(atp_partial);
    }
    atp.save_path = dir;
    ses.async_add_torrent(std::move(atp));

    // this is the handle we'll set once we get the notification of it being
    // added
    lt::torrent_handle h;

    std::signal(SIGINT, &sighandler);

    bool done = false;
    for (;;) {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);

        if (shut_down) {
            shut_down = false;
            const std::vector<lt::torrent_handle> handles = ses.get_torrents();
            if (handles.size() == 1) {
                handles[0].save_resume_data(
                    lt::torrent_handle::only_if_modified |
                    lt::torrent_handle::save_info_dict);
                done = true;
            }
        }

        for (const lt::alert* a : alerts) {
            if (const lt::add_torrent_alert* at =
                    lt::alert_cast<lt::add_torrent_alert>(a)) {
                h = at->handle;
            }
            if (should_write_torrent_file) {
                if (const lt::metadata_received_alert* md =
                        lt::alert_cast<lt::metadata_received_alert>(a)) {
                    write_torrent_file(torrent_file_path,
                                       md->handle.torrent_file());
                    should_write_torrent_file = false;
                }
            }
            // if we receive the finished alert or an error, we're done
            if (lt::alert_cast<lt::torrent_finished_alert>(a)) {
                h.save_resume_data(lt::torrent_handle::only_if_modified |
                                   lt::torrent_handle::save_info_dict);
                done = true;
            }
            if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                std::cout << a->message() << '\n';
                done = true;
                h.save_resume_data(lt::torrent_handle::only_if_modified |
                                   lt::torrent_handle::save_info_dict);
            }

            // when resume data is ready, save it
            if (const lt::save_resume_data_alert* rd =
                    lt::alert_cast<lt::save_resume_data_alert>(a)) {
                std::ofstream of(dir + torrent_name + ".resume",
                                 std::ios_base::binary);
                of.unsetf(std::ios_base::skipws);
                const std::vector<char> b = write_resume_data_buf(rd->params);
                of.write(b.data(), static_cast<int>(b.size()));
                if (done)
                    goto done;
            }

            if (lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
                if (done)
                    goto done;
            }

            if (const lt::state_update_alert* st =
                    lt::alert_cast<lt::state_update_alert>(a)) {
                if (st->status.empty())
                    continue;

                // we only have a single torrent, so we know which one
                // the status is for
                const lt::torrent_status& s = st->status[0];
                std::cout << '\r' << "Leech " << state(s.state) << ": "
                          << (s.download_payload_rate / 1000) << " kB/s "
                          << (s.total_done / 1000) << " kB ("
                          << (s.progress_ppm / 10000) << "%) downloaded ("
                          << s.num_peers << " peers)\x1b[K";
                std::cout.flush();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // ask the session to post a state_update_alert, to update our
        // state output for the torrent
        ses.post_torrent_updates();

        // save resume data once every 30 seconds
        if (clk::now() - last_save_resume > std::chrono::seconds(30)) {
            h.save_resume_data(lt::torrent_handle::only_if_modified |
                               lt::torrent_handle::save_info_dict);
            last_save_resume = clk::now();
        }
    }

done:
    std::cout << "\rLeech complete. Saving session state...\x1b[K\n";
    {
        std::ofstream of((dir + torrent_name + ".session"),
                         std::ios_base::binary);
        of.unsetf(std::ios_base::skipws);
        const std::vector<char> b = write_session_params_buf(
            ses.session_state(), lt::save_state_flags_t::all());
        of.write(b.data(), static_cast<int>(b.size()));
    }

    std::cout << "Closing leeching client...\n";

    // Store the output path of the leeched bare repository
    const std::string repo_path = dir + torrent_name;
    if (output_path && output_path_size > 0) {
        g_snprintf(output_path, output_path_size, "%s", repo_path.c_str());
    }

    gittor_seed_start(torrent_name.c_str());

    return 0;
} catch (std::exception& e) {
    std::cerr << "Error Leeching: " << e.what() << '\n';
    return 1;
}
