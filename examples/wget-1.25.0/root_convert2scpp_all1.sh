#!/bin/bash

# Get the directory of the current script
SCRIPT_DIR="$(dirname "$0")"

date
date &> root_convert2scpp_all1_out.txt
echo -e "" &>> root_convert2scpp_all1_out.txt

set -x

echo -e "" &>> root_convert2scpp_all1_out.txt
"$SCRIPT_DIR/root_convert2scpp1.sh" "./wget-1.25.0/src/main.c" "./wget-1.25.0/src/connect.c" "./wget-1.25.0/src/css_.c" "./wget-1.25.0/src/build_info.c" "./wget-1.25.0/src/hsts.c" "./wget-1.25.0/src/ftp-ls.c" "./wget-1.25.0/src/ftp.c" "./wget-1.25.0/src/hash.c" "./wget-1.25.0/src/netrc.c" "./wget-1.25.0/src/css-url.c" "./wget-1.25.0/src/xattr.c" "./wget-1.25.0/src/warc.c" "./wget-1.25.0/src/utils.c" "./wget-1.25.0/src/url.c" "./wget-1.25.0/src/spider.c" "./wget-1.25.0/src/retr.c" "./wget-1.25.0/src/recur.c" "./wget-1.25.0/src/res.c" "./wget-1.25.0/src/progress.c" "./wget-1.25.0/src/openssl.c" "./wget-1.25.0/src/metalink.c" "./wget-1.25.0/src/log.c" "./wget-1.25.0/src/init.c" "./wget-1.25.0/src/http-ntlm.c" "./wget-1.25.0/src/ptimer.c" "./wget-1.25.0/src/html-url.c" "./wget-1.25.0/src/html-parse.c" "./wget-1.25.0/src/host.c" "./wget-1.25.0/src/ftp-opie.c" "./wget-1.25.0/src/ftp-basic.c" "./wget-1.25.0/src/exits.c" "./wget-1.25.0/src/cookies.c" "./wget-1.25.0/src/convert.c" "./wget-1.25.0/src/http.c" "./wget-build/src/version.c" &>> root_convert2scpp_all1_out.txt

rm *.o

# "./wget-1.25.0/src/css.c" 

set +x

echo -e "" &>> root_convert2scpp_all1_out.txt

date &>> root_convert2scpp_all1_out.txt
date

echo -n "errors: "
cat root_convert2scpp_all1_out.txt | grep -c -i ": error: "
echo -n "warnings: "
cat root_convert2scpp_all1_out.txt | grep -c -i ": warning: "

