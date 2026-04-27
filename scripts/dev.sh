#!/usr/bin/env bash
set -u

npm run dev --prefix server &
SERVER_PID=$!

npm run dev --prefix client &
CLIENT_PID=$!

EXIT_CODE=0
wait "$SERVER_PID" || EXIT_CODE=$?
wait "$CLIENT_PID" || EXIT_CODE=$?

exit "$EXIT_CODE"
