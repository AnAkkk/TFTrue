[![Build Status](https://travis-ci.org/AnAkkk/TFTrue.svg?branch=public)](https://travis-ci.org/AnAkkk/TFTrue)

# Credits

AnAkkk: Current TFTrue developer  
Didrole: Retired developer (2.x)  
Red Comet: Original TFTrue developer

# How to compile

For now, the project uses qmake which means it needs to be opened in Qt Creator.
Is it recommended to compile it with MSVC 2013 on Windows, gcc 4.9 or clang 3.5 on Linux but it might work with older and/or newer versions.

You will need to adjust SOURCE_DIR in TFTrue.pro to point to the source SDK 2013 "mp/src" directory (https://github.com/AnAkkk/source-sdk-2013).

# Additional dependencies

ModuleScanner and FunctionRoute were made by Didrole. The source code is not available yet but should be *soon*.
The libraries are part of the repository for now.
jsoncpp (https://github.com/open-source-parsers/jsoncpp) is needed too and included in this repository.

# Configuration

### Server:
tftrue_gamedesc  
Server owners can use this cvar to set specific text in the game description of their server.
There is a 40 character max, and your text will appear after 'TFTrue'

tftrue_freezecam  
Enables/disables the freeze cam. Default is 1 (enabled).

tftrue_maxfov  
Sets the maximum fov the players will be able to set with the "!fov" chat command. Default is 90.

tftrue_no_hats  
Enables/disables the hats. Default is 0 (hats enabled).

tftrue_no_misc  
Enables/disables the misc items. Default is 0 (misc items enabled).

tftrue_no_action  
Enables/disables action items. Default is 0 (action items enabled).

tftrue_whitelist_id  
Sets a whitelist id from whitelist.tf. Default is -1 (disabled).

tftrue_tournament_config  
Sets specific league configs. It will auto download the configs and execute them depending of the map type.
If you use this, you do not need to set tftrue_whitelist_id as mp_tournament_whitelist is already set in the league configs.
0: None
1: ETF2L 6on6
2: ETF2L 9on9
Default is 1.

tftrue_tv_autorecord  
Turn on auto STV recording when both teams are ready in tournament mode. It will stops when the win conditions are reached. Default is 1 (enabled).

tftrue_bunnyhop  
Turn on/off bunny hopping. The opening speed of the doors will be changed to the max value as well to prevent yourself getting stuck while bunny hopping.
This also enables "pogo stick jumping", so you can just hold down space to bunny hop.
It will let you jump while ducking as well to make bunnyhopping easier. Default is 0 (disabled).

tftrue_tv_demos_path  
Lets you define a folder inside "tf" where you want the demos recorded by tftrue_tv_autorecord to be stored. The folder will be automatically created if the CVar is set.
They will be stored in "tf" by default.

tftrue_tv_prefix  
Sets the prefix to add in the auto recorded demo names.

tftrue_unpause_delay  
Sets the delay before someone can unpause the game after it has been paused. Default is 2.

tftrue_logs_apikey  
Sets the API key to upload logs to logs.tf (requires mp_tournament). It will automatically flush the log before upload, so any content that is still in memory will be wrote to the log.

tftrue_logs_prefix  
Sets the prefix to add in the log name when uploading to logs.tf.

tftrue_logs_roundend  
Whether to upload logs at every round end or just when a team wins the map. Default is 0 (just when a team wins the map).

tftrue_restorestats  
Keeps the player stats on disconnect (Score, Kills, Deaths, etc...) and restore them when he reconnects in both scoreboard and A2S_PLAYER queries (e.g. View Game Info in the Server Browser).
Default is 1 (enabled).

tftrue_logs_includebuffs  
Includes buffs within the player_healed event in the logs.
Default is 1 (enabled).

tftrue_logs_accuracy  
Logs accuracy stats in the logs (shots fired/hit). It can potentially cause performance issues with some servers.
Default is 0 (disabled).

### Client:

say !tftrue  
In the chat, you can type !tftrue (or bind a key to say !tftrue), and it'll display informations about the
TFTrue version and the current CVar values.

say !fov  
In the chat, you can type !fov xxx, where xxx is a value between 75 and tftrue_maxfov. This will set your fov to this value.

say !log  
In the chat, you can type !log to view the last log which has been uploaded to logs.tf

setinfo tftrue_fov  
Same as the chat command to set the fov, but you can set it in your config to automatically set the fov on server connection.
Example: setinfo tftrue_fov 130

say !speedmeter [on/off]  
In the chat, you can type !speedmeter [on/off] while tftrue_bunnyhop is enabled, this will print your bunny hopping speed in the middle of your screen.

#Other Features:

- The map will now automatically be reloaded when sv_pure or tv_enable value is changed, as these CVars need a map change to work.
- While using rcon status/status, it will display the plugins currently loaded on the server (prevents server owners to cheat with plugins) when the tournament mode is enabled. status will display current TFTrue settings as well.
- Logs.tf support
- Tournament classlimits working without tournament mode (for pub servers)
- Tournament whitelists working without tournament mode (for pub servers)
- Changing mp_tournament_whitelist will automatically reload the whitelist without requiring mp_tournament_restart
- Console messages when allowing/removing whitelist items are removed to prevent spam
- You can write in the chat more than once when the game is paused
- Removes the block on plugin_load that prevents you from using it after a map has been loaded

NOTE: mp_tournament_whitelist will always be reset to TFTrue_item_whitelist.txt, this is perfectly normal. TFTrue will generate a new whitelist based on the whitelist that was set through mp_tournament_whitelist + TFTrue settings. 
