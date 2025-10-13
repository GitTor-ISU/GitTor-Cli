#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/write_resume_data.hpp>
#include "examples/tor.h"

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

}  // anonymous namespace

int torrent_example() try {
    auto magnet_uri =
        "magnet:?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big+"
        "Buck+Bunny&tr=udp%3A%2F%2Fexplodie.org%3A6969&tr=udp%3A%2F%2Ftracker."
        "coppersurfer.tk%3A6969&tr=udp%3A%2F%2Ftracker.empire-js.us%3A1337&tr="
        "udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969&tr=udp%3A%2F%"
        "2Ftracker.opentrackr.org%3A1337&tr=wss%3A%2F%2Ftracker.btorrent.xyz&"
        "tr=wss%3A%2F%2Ftracker.fastcast.nz&tr=wss%3A%2F%2Ftracker."
        "openwebtorrent.com&ws=https%3A%2F%2Fwebtorrent.io%2Ftorrents%2F&xs="
        "https%3A%2F%2Fwebtorrent.io%2Ftorrents%2Fbig-buck-bunny.torrent";

    const std::string dir = "/tmp/tor/";

    // load session parameters
    auto session_params = load_file((dir + ".session").c_str());
    lt::session_params params = session_params.empty()
                                    ? lt::session_params()
                                    : lt::read_session_params(session_params);
    params.settings.set_int(lt::settings_pack::alert_mask,
                            lt::alert_category::error |
                                lt::alert_category::storage |
                                lt::alert_category::status);

    lt::session ses(params);
    clk::time_point last_save_resume = clk::now();

    // load resume data from disk and pass it in as we add the magnet link
    auto buf = load_file((dir + ".resume_file").c_str());

    lt::add_torrent_params magnet = lt::parse_magnet_uri(magnet_uri);
    if (buf.size()) {
        lt::add_torrent_params atp = lt::read_resume_data(buf);
        if (atp.info_hashes == magnet.info_hashes)
            magnet = std::move(atp);
    }
    magnet.save_path = dir;  // save in current dir
    ses.async_add_torrent(std::move(magnet));

    // this is the handle we'll set once we get the notification of it being
    // added
    lt::torrent_handle h;

    std::signal(SIGINT, &sighandler);

    // set when we're exiting
    bool done = false;
    for (;;) {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);

        if (shut_down) {
            shut_down = false;
            auto const handles = ses.get_torrents();
            if (handles.size() == 1) {
                handles[0].save_resume_data(
                    lt::torrent_handle::only_if_modified |
                    lt::torrent_handle::save_info_dict);
                done = true;
            }
        }

        for (lt::alert const* a : alerts) {
            if (auto at = lt::alert_cast<lt::add_torrent_alert>(a)) {
                h = at->handle;
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
            if (auto rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
                std::ofstream of(dir + ".resume_file", std::ios_base::binary);
                of.unsetf(std::ios_base::skipws);
                auto const b = write_resume_data_buf(rd->params);
                of.write(b.data(), static_cast<int>(b.size()));
                if (done)
                    goto done;
            }

            if (lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
                if (done)
                    goto done;
            }

            if (auto st = lt::alert_cast<lt::state_update_alert>(a)) {
                if (st->status.empty())
                    continue;

                // we only have a single torrent, so we know which one
                // the status is for
                lt::torrent_status const& s = st->status[0];
                std::cout << '\r' << state(s.state) << ' '
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
    std::cout << "\nsaving session state" << '\n';
    {
        std::ofstream of(dir + ".session", std::ios_base::binary);
        of.unsetf(std::ios_base::skipws);
        auto const b = write_session_params_buf(ses.session_state(),
                                                lt::save_state_flags_t::all());
        of.write(b.data(), static_cast<int>(b.size()));
    }

    std::cout << "\ndone, shutting down" << '\n';
    return 0;
} catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
}
