#!/bin/bash
# Run in root of repo
rm -rf hl2sdk-*
git clone https://github.com/alliedmodders/hl2sdk --branch tf2     --single-branch hl2sdk-tf2  --depth 1
git clone https://github.com/alliedmodders/hl2sdk --branch sdk2013 --single-branch hl2sdk-2013 --depth 1

# Changes from SDK2013:
# copy libsteam_api.so
cp ./hl2sdk-2013/lib/public/linux32/libsteam_api.so ./bin/libsteam_api.so
# Fix conflicting return type with GetPlayerMins/GetPlayerMaxs
# https://github.com/alliedmodders/hl2sdk/pull/99
rm  ./hl2sdk-tf2/game/shared/gamemovement.h
# permalink to this commit
curl https://raw.githubusercontent.com/sapphonie/hl2sdk/10934b8c9edf900850b1d85f1b9eff90addc0084/game/shared/gamemovement.h -o ./hl2sdk-tf2/game/shared/gamemovement.h
# Copy steamworks funcs from 2013 to tf2 sdk
cp ./hl2sdk-2013/public/steam/* ./hl2sdk-tf2/public/steam/ -Rfv
# Replay stuff for replayserver->GetGameServer() - superceded by engine->GetIServer()
# cp ./hl2sdk-2013/common/replay/ ./hl2sdk-tf2/common/ -Rfva

rm -rf hl2sdk-2013
