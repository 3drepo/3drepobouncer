3D Repo Bouncer Library v1.8.0

This is the internal library used for 3D Repo GUI.

====================== Directory Listing ========================
 > bin
      3drepobouncerClient - Command line tool to utilise 
      the client (and its required DLLs)
 > include
      header files for 3drepobouncer library
 > lib
	 libraries for 3drepobouncer library
 > license
      licensing information

=========================== To Run ==============================
3D Repo Bouncer is primarily a library. However you can run the
command line program provided which expose a small amount of
features within the library, designed for server deployment

To exploit the full features of the library as a user, please 
download a copy of our desktop client - 3D Repo GUI.

=========================== Contact ==============================
If you need any help, please contact support@3drepo.org, we
look forward to hear from you.

========================= Improvements =========================
* (ISSUE #92) Fix crashes when database connection is lost
* (ISSUE #157) Groups collection is now removed when a project is deleted
* (ISSUE #167) IFCOpenShell imports now use original material names as default
* (ISSUE #168) Log file is flushed after every log call

========================= New Features =========================
* (ISSUE #160) Project permissions are now more fine grained
* (ISSUE #161) The ability to use Assimp/IFCOpenShell to import ifc files based on config.
* (ISSUE #163 #164) Some administration and test changes
* (ISSUE #166) Ability to reset changelist on a RepoScene is added
