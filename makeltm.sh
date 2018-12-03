#!/bin/bash -e

if [[ "$#" -ne "1" ]] || ! grep -q '\.ltm$' <<< "$1"; then
	echo "usage: $0 filename.ltm" >&2
	exit 2
fi

# normalize files so timestamps and permissions always match.
for FILE in tas/*; do
    TIME=$(git log --pretty=format:%cd -n 1 --date=iso -- "$FILE")
    TIME=$(date --date="$TIME" +%Y%m%d%H%M.%S)
    touch -m -t "$TIME" "$FILE"
    chmod 664 "$FILE"
done

# force the TAR file's group/user names to match mine, and tell gzip not to record metadata about the tar file itself.
tar cC tas --group=ben --owner=ben inputs config.ini annotations.txt | gzip -9n > "$1"
