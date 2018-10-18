#!/bin/bash

set -e

ITEMS=()
for i in */ ; do
    if [ "${i:0:1}" == "." ]; then
        continue
    fi

    i="${i%%/}"
    echo " + Copy: $i"
    ITEMS+=("$i")

    mkdir -p ".release/$i/src"
    cp -r "$i/." ".release/$i/src"
    if [ -e ".release/$i/src/README.md" ]; then
        mv ".release/$i/src/README.md" ".release/$i/README.md"
    fi

    if [ ! -e ".release/$i/kendryte-package.json" ]; then
        cat > ".release/$i/kendryte-package.json" <<JSON_EOF
{
	// cmake config file
	"\$schema": "vscode://schemas/CMakeLists",
	"name": "${i}",
	"version": "1.0.0",
	"type": "example",
	"source": [
		"src/*.c",
		"src/*.cpp",
		"src/*.h",
		"src/*.hpp",
	],
	"c_flags": [
		"-std=gnu11",
	],
	"cpp_flags": [
		"-std=gnu++17",
	],
    "dependency": {
        "kendryte-sdk": "https://s3.cn-northwest-1.amazonaws.com.cn/kendryte-ide/package-manager/internal/kendryte-sdk-standalone-1.0.0.tar.gz"
    },
}
JSON_EOF
    fi
done

cd .release
for I in "${ITEMS[@]}" ; do
    echo " + Compress: $I"
    tar -czf "$I.tar.gz" "$I" --no-acls --no-xattrs --remove-files
    rm -rf
done

echo ""
echo ""
for I in "${ITEMS[@]}" ; do
    cat <<LINE_EOF
    {"name": "${I}", "versions": [
        {"versionName": "1.0.0", "downloadUrl": "examples/${I}.tar.gz"},
    ]},
LINE_EOF
done
