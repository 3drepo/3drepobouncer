3D Repo Bouncer Library v1.6.0

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
* (ISSUE #133) Bug Fix cross database federation
* (ISSUE #138) User BSON values will no longer be lost when modified
* (ISSUE #131, #137, #139, #140) Improved selection tree support
* (ISSUE #141) Project settings values will no longer be lost when modified
* (ISSUE #143) Command line file imports now accepts commit tag/description
* (ISSUE #147) Fixed camera positions
* (ISSUE #151) Fixed offset calculations on federated projects
* (ISSUE #153) Fixed fbx rotations that went wrong after #143
* (ISSUE #154) Small bug fix on Matrix Vector multiplication

========================= New Features =========================
* (ISSUE #132) Federation creation API exposed on client
* (ISSUE #134) Ability to read/update user licensing  information
* (ISSUE #136) FBX imports will be automatically rotated
* (ISSUE #142) Supports "hidden by default" geometries
* (ISSUE #144, #145, #152, #156, #158) Improved IFC geometry support




