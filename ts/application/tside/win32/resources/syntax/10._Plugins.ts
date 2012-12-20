# Plugins provide an easy way to extend Trigger Script.  With a plugin,
# new functions and action triggers can be written in any language
# supporting the C calling convention.  When either the TS IDE or TSI
# command line tool starts, plugins are loaded from the program directory
# as well as the user's "My Trigger Scripts\Plugins" directory.
# Directories listed in the 'ts_plugin' environment variable will also be
# searched for plugins.  The 'ts_plugin' environment variable has the
# form:

# ts_plugin = <path1>;<path2>;...;<pathn>

# Plugins are determined by their extension.  Any DLL with the extension
# <name>.ts.dll, will be loaded by TS.

# When a plugin is loaded, the IDE will automatically build a list of
# functions contained in it as well as documentation for those functions.
# Opening the reference window in the TS IDE will make the documentation
# for the plugin visible.

