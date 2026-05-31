#pragma once

#include <string>
#include <vector>

struct Strategy
{
    std::wstring id;
    std::wstring displayName;
    std::wstring description;
    std::wstring args;
};

inline std::vector<Strategy> GetAllStrategies()
{
    std::vector<Strategy> strategies;

    {
        Strategy s;
        s.id = L"general";
        s.displayName = L"general";
        s.description = L"general.bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-split-seqovl=568 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_4pda_to.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-split-seqovl=568 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_4pda_to.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n3 --dpi-desync-split-seqovl=568 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_4pda_to.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=12 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n2";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt1";
        s.displayName = L"general (ALT)";
        s.description = L"general (ALT).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=fake,fakedsplit --dpi-desync-repeats=6 --dpi-desync-fooling=ts --dpi-desync-fakedsplit-pattern=0x00 --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=fake,fakedsplit --dpi-desync-repeats=6 --dpi-desync-fooling=ts --dpi-desync-fakedsplit-pattern=0x00 --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,fakedsplit --dpi-desync-repeats=6 --dpi-desync-fooling=ts --dpi-desync-fakedsplit-pattern=0x00 --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,fakedsplit --dpi-desync-repeats=6 --dpi-desync-fooling=ts --dpi-desync-fakedsplit-pattern=0x00 --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,fakedsplit --dpi-desync-repeats=6 --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n4 --dpi-desync-fooling=ts --dpi-desync-fakedsplit-pattern=0x00 --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=12 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n3";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt2";
        s.displayName = L"general (ALT2)";
        s.description = L"general (ALT2).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=multisplit --dpi-desync-split-seqovl=652 --dpi-desync-split-pos=2 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=multisplit --dpi-desync-split-seqovl=652 --dpi-desync-split-pos=2 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-split-seqovl=652 --dpi-desync-split-pos=2 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-split-seqovl=652 --dpi-desync-split-pos=2 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n3 --dpi-desync-split-seqovl=652 --dpi-desync-split-pos=2 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=12 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n2";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt3";
        s.displayName = L"general (ALT3)";
        s.description = L"general (ALT3).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=fake,hostfakesplit --dpi-desync-fake-tls-mod=rnd,dupsid,sni=www.google.com --dpi-desync-hostfakesplit-mod=host=www.google.com,altorder=1 --dpi-desync-fooling=ts --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=fake,hostfakesplit --dpi-desync-fake-tls-mod=rnd,dupsid,sni=www.google.com --dpi-desync-hostfakesplit-mod=host=www.google.com,altorder=1 --dpi-desync-fooling=ts --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,hostfakesplit --dpi-desync-fake-tls-mod=rnd,dupsid,sni=ya.ru --dpi-desync-hostfakesplit-mod=host=ya.ru,altorder=1 --dpi-desync-fooling=ts --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,hostfakesplit --dpi-desync-fake-tls-mod=rnd,dupsid,sni=ya.ru --dpi-desync-hostfakesplit-mod=host=ya.ru,altorder=1 --dpi-desync-fooling=ts --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,hostfakesplit --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n4 --dpi-desync-fake-tls-mod=rnd,dupsid,sni=ya.ru --dpi-desync-hostfakesplit-mod=host=ya.ru,altorder=1 --dpi-desync-fooling=ts --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=10 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n4";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt4";
        s.displayName = L"general (ALT4)";
        s.description = L"general (ALT4).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=fake,multisplit --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=1000 --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=fake,multisplit --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=1000 --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,multisplit --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=1000 --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,multisplit --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=1000 --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,multisplit --dpi-desync-repeats=6 --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n3 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=1000 --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=10 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n2";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt5";
        s.displayName = L"general (ALT5)";
        s.description = L"general (ALT5).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-l3=ipv4 --filter-tcp=80,443,2053,2083,2087,2096,8443 --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=syndata,multidisorder --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=syndata,multidisorder --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n4 --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=14 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n3";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt6";
        s.displayName = L"general (ALT6)";
        s.description = L"general (ALT6).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n3 --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=12 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n2";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt7";
        s.displayName = L"general (ALT7)";
        s.description = L"general (ALT7).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=multisplit --dpi-desync-split-pos=2,sniext+1 --dpi-desync-split-seqovl=679 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=multisplit --dpi-desync-split-pos=2,sniext+1 --dpi-desync-split-seqovl=679 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=multisplit --dpi-desync-split-pos=2,sniext+1 --dpi-desync-split-seqovl=679 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=syndata --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=syndata --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n4 --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=12 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n2";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt8";
        s.displayName = L"general (ALT8)";
        s.description = L"general (ALT8).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=fake --dpi-desync-fake-tls-mod=none --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=fake --dpi-desync-fake-tls-mod=none --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-fake-tls-mod=none --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-fake-tls-mod=none --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n3 --dpi-desync-fake-tls-mod=none --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=12 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n2";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt9";
        s.displayName = L"general (ALT9)";
        s.description = L"general (ALT9).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=hostfakesplit --dpi-desync-repeats=4 --dpi-desync-fooling=ts --dpi-desync-hostfakesplit-mod=host=www.google.com --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=hostfakesplit --dpi-desync-repeats=4 --dpi-desync-fooling=ts --dpi-desync-hostfakesplit-mod=host=www.google.com --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=hostfakesplit --dpi-desync-repeats=4 --dpi-desync-fooling=ts,md5sig --dpi-desync-hostfakesplit-mod=host=ozon.ru --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=hostfakesplit --dpi-desync-repeats=4 --dpi-desync-fooling=ts --dpi-desync-hostfakesplit-mod=host=ozon.ru --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=hostfakesplit --dpi-desync-repeats=4 --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n3 --dpi-desync-fooling=ts --dpi-desync-hostfakesplit-mod=host=ozon.ru --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=12 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n2";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt10";
        s.displayName = L"general (ALT10)";
        s.description = L"general (ALT10).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fooling=ts --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-tls-mod=none --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fooling=ts --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fooling=ts --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_4pda_to.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fooling=ts --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_4pda_to.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n3 --dpi-desync-fooling=ts --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_4pda_to.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=12 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n2";
        strategies.push_back(s);
    }

    {
        Strategy s;
        s.id = L"alt11";
        s.displayName = L"general (ALT11)";
        s.description = L"general (ALT11).bat";
        s.args =
            L"--wf-tcp=80,443,2053,2083,2087,2096,8443,{GAME_TCP} "
            L"--wf-udp=443,19294-19344,50000-50100,{GAME_UDP} "
            L"--filter-udp=443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-udp=19294-19344,50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-discord=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-fake-stun=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-repeats=6 --new "
            L"--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=fake,multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-fooling=ts --dpi-desync-repeats=8 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=443 --hostlist=\"{LISTS}list-google.txt\" --ip-id=zero --dpi-desync=fake,multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-fooling=ts --dpi-desync-repeats=8 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_www_google_com.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_www_google_com.bin\" --new "
            L"--filter-tcp=80,443 --hostlist=\"{LISTS}list-general.txt\" --hostlist=\"{LISTS}list-general-user.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,multisplit --dpi-desync-split-seqovl=664 --dpi-desync-split-pos=1 --dpi-desync-fooling=ts --dpi-desync-repeats=8 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_max_ru.bin\" --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_max_ru.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp=443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{BIN}quic_initial_www_google_com.bin\" --new "
            L"--filter-tcp=80,443,8443 --ipset=\"{LISTS}ipset-all.txt\" --hostlist-exclude=\"{LISTS}list-exclude.txt\" --hostlist-exclude=\"{LISTS}list-exclude-user.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,multisplit --dpi-desync-split-seqovl=664 --dpi-desync-split-pos=1 --dpi-desync-fooling=ts --dpi-desync-repeats=8 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_max_ru.bin\" --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_max_ru.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-tcp={GAME_TCP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake,multisplit --dpi-desync-any-protocol=1 --dpi-desync-cutoff=n4 --dpi-desync-split-seqovl=664 --dpi-desync-split-pos=1 --dpi-desync-fooling=ts --dpi-desync-repeats=8 --dpi-desync-split-seqovl-pattern=\"{BIN}tls_clienthello_max_ru.bin\" --dpi-desync-fake-tls=\"{BIN}stun.bin\" --dpi-desync-fake-tls=\"{BIN}tls_clienthello_max_ru.bin\" --dpi-desync-fake-http=\"{BIN}tls_clienthello_max_ru.bin\" --new "
            L"--filter-udp={GAME_UDP} --ipset=\"{LISTS}ipset-all.txt\" --ipset-exclude=\"{LISTS}ipset-exclude.txt\" --ipset-exclude=\"{LISTS}ipset-exclude-user.txt\" --dpi-desync=fake --dpi-desync-repeats=10 --dpi-desync-any-protocol=1 --dpi-desync-fake-unknown-udp=\"{BIN}quic_initial_dbankcloud_ru.bin\" --dpi-desync-cutoff=n4";
        strategies.push_back(s);
    }

    return strategies;
}