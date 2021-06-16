const fs = require("fs");
const path = require("path");

/**
 * ============== Constant Declarations - edit as necessary ==============
 */
const verbose = true;
const exts = [".tx", ".txv", ".dll", ".so"];

const externalLibraries = [
	{
		rootEnvVar: "MONGO_ROOT",
		subPath: ["lib"],
		extensions: exts
	},
	{
		rootEnvVar: "ASSIMP_ROOT",
		subPath: ["bin", "lib"],
		extensions: [...exts, ".5"]
	},
	{
		rootEnvVar: "ODA_ROOT",
		subPath: ["exe/vc14_amd64dll", "bin/lnxX64_8.3dll"],
		subFolder:  ["exe/vc14_amd64dll/CSV", "bin/lnxX64_8.3dll/CSV"],
		extensions: [...exts, ".txt"]
	},
	{
		rootEnvVar: "THRIFT_ROOT",
		subPath: ["lib"],
		extensions: [exts, ".0"]
	},
	{
		rootEnvVar: "OCCT_ROOT",
		subPath: ["lib", "exe/vc14_amd64dll", "bin/lnxX64_8.3dll"],
		extensions: [...exts, ".7"]
	},
	{
		rootEnvVar: "IFCOPENSHELL_ROOT",
		subPath: ["lib"],
		extensions: exts
	},
	{
		rootEnvVar: "SYNCHRO_READER_ROOT",
		subPath: ["lib"],
		subFolder: ["plugins"],
		extensions: exts
	},
	{
		rootEnvVar: "BOOST_ROOT",
		subPath: ["lib", "lib64-msvc-14.0"],
		extensions: exts
	},
	{
		rootEnvVar: "BOUNCER_ROOT",
		subPath: ["lib", "bin"],
		extensions: [...exts, ".exe", ""]
	}
];

/**
 * =======================================================================
 */

// https://stackoverflow.com/a/64255382
const copyDir = (src, dest) => {
	verbose && console.log(`Copying folder ${src} to ${dest}...`);
    fs.mkdirSync(dest, { recursive: true });
    const entries = fs.readdirSync(src, { withFileTypes: true });

    for (let entry of entries) {
        const srcPath = path.join(src, entry.name);
        const destPath = path.join(dest, entry.name);

        entry.isDirectory() ?
            copyDir(srcPath, destPath) :
            fs.copyFileSync(srcPath, destPath);
    }
};

const folderPath = process.argv[2] || `3drepobouncer`
/*
 uncomment when we upgrade nodejs.
if(fs.existsSync(folderPath)) {
	fs.rmSync(folderPath, {recursive: true});
}
	*/
fs.mkdirSync(folderPath);

externalLibraries.forEach(({rootEnvVar, subPath, subFolder, extensions}) => {
	verbose && console.log(`Processing ${rootEnvVar}...`);
	const rootPath = process.env[rootEnvVar];
	verbose && console.log(`\tPath is: ${rootPath}...`);
	if(!( rootPath && fs.existsSync(rootPath))) {
		console.log(`\tCould not find env var ${rootEnvVar}. Skipping...`);
		return;
	}

	if(subPath && subPath.length) {
		subPath.forEach((subDir) => {
			const fullPath = path.join(rootPath, subDir);
			verbose && console.log(`\tSub dir: ${subDir}... (${fullPath})`);
			if(fs.existsSync(fullPath)) {
				const files = fs.readdirSync(fullPath);
				files.forEach((file) => {
					const fileExt = path.extname(file);
					if (extensions.includes(fileExt)) {
						const filePath = path.join(fullPath, file);
						verbose && console.log(`\tCopying file: ${filePath}...`);
						const destPath = path.join(folderPath, file);
						fs.copyFileSync(filePath, destPath);
					}
				});
			} else {
				verbose && console.log(`\t${subDir} does not exist. skipping...`);
			}
		});
	}

	if(subFolder && subFolder.length) {
		subFolder.forEach((subDir) => {
			const fullPath = path.join(rootPath, subDir);
			verbose && console.log(`\tFound sub folder: ${fullPath}`);
			if(fs.existsSync(fullPath)) {
				copyDir(fullPath, path.join(folderPath, path.basename(subDir)));
			}
		});
	}


});

