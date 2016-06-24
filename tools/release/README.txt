3D Repo Bouncer Library v1.1.0

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
* (ISSUE #116) UV coordinates are now retained during SRC export
* (ISSUE #121) Remove automatic owner assignment
* (ISSUE #125) Fixed triangulation on SRC export
* (ISSUE #126) project.stash.json_mpc.[files|chunks] are now removed if project is deleted
* (ISSUE #127) Fixed material mapping on the json export.


========================= New Features =========================
* (ISSUE #80) Ability to revert(clean up) a commit if an error occurred
* (ISSUE #115) Mesh Instancing is no longer supported (meshes are duplicated)
* (ISSUE #118) The directory where the log is written is now displayed
* (ISSUE #119) The bouncer worker now puts log files under a folder with request ID as name
* (ISSUE #120) Missing textures is now indicated during import
* (ISSUE #123) Selection Tree generation can now be called using 3drepobouncerClient.
* (ISSUE #124) Improved error reporting on Bouncer worker.
* (ISSUE #129) Added support for BOOST 1.61
* (ISSUE #130) Improved support for IFC.


