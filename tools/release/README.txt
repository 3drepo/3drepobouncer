3D Repo Bouncer Library v 1.0(Release Client 1)

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
* (ISSUE #111) Token is no longer created if the database connection fails no credentials
* (ISSUE #108) Removed unneccessary header includes and obscured RepoController implementation from view
* (ISSUE #106) Split Mesh has been fixed for large meshes and Mesh remapping has been moved to its own class
* (ISSUE #100, #101, #105) Multipart(Optimised graph) fixes
* (ISSUE #59) User refactoring to remove obsoleted fields

========================= New Features =========================
* (ISSUE #113) A default "owner role is automatically created with project commits
* (ISSUE #110) RepoClient will now log to file
* (ISSUE #109) RepoToken and RepoCredentials have now merged into a single class
* (ISSUE #107) X3D generation is removed from bouncer as it is no longer neccessary
* (ISSUE #104) Metadata improvements
* (ISSUE #103) The selection tree is now stored in GridFS upon commit
* (ISSUE #102) More controller functionality has been exposed on the client
* (ISSUE #99) Bouncer now supports 32bit Assimp again, with a warning message
* (ISSUE #93) Faces re-ordering for buffer support
* (ISSUE #75) GLTF generation



