3D Repo Bouncer Library v 1.0(beta)

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

============================ Improvments =============================
* (Issue #27) Increased speed of file imports
* (Issue #29) Fix reorientation function for FBX models
* (Issue #47) Optimised graph generation on longer relies on ASSIMP
* (Issue #69) Fix issues with textured models
* (Issue #70) Removed Spurious log messages during file import
* (Issue #72, #81) Fetch head will now fetch the latest COMPLETE version
* (Issue #73, #74) Fix some memory leaks
* (Issue #84) Add support for Boost v.1.60
* (Issue #86) Removed default node searches within RepoScene (you will now need to specify which graph to look for)
* (Issue #87) Fix crashes due to user connecting to the database without credentials
* (Issue #89) Tone down errors related to roles
* (Issue #90) Remove false warnings due to numerical inaccuracies
* (Issue #91) Commiting new versions of models will no longer reset project settings
* (Issue #95) Added workaround for generating optimised graphs with texture (textures should now be visible on the web)

============================= New Features ============================
* (Issue #46) Ability to regenerate optimised graph
* (Issue #51) Ability to remove a project in a single function call 
* (Issue #67, #76) Ability to generate X3D exports for web viewing
* (Issue #79) Map node now stores the API key if required
* (Issue #94) Support textured models with no UV coordinates (where the mesh has the same 2D dimension as the texture image)
* (Issue #96) Models are now shifted to the origin for viewing, and will be shifted accordingly when federated.




