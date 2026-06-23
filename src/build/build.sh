#!/bin/bash
# Build Bangla Keyboard Layout for Mac  (.pkg + .dmg)
set -e
export COPYFILE_DISABLE=1
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
PKGID="com.biswashost.bangla-keyboard"
VERSION="${1:-1.0.0}"
BUILD="$ROOT/.build"; DIST="$ROOT/dist"
rm -rf "$BUILD"; mkdir -p "$BUILD/payload/Library/Keyboard Layouts" "$BUILD/payload/Library/Fonts" "$DIST"

cp "$ROOT/src/keylayouts/"*.keylayout "$ROOT/src/keylayouts/"*.icns "$BUILD/payload/Library/Keyboard Layouts/"
cp "$ROOT/src/fonts/"*.ttf "$BUILD/payload/Library/Fonts/"
find "$BUILD/payload" -name '._*' -delete; find "$BUILD/payload" -name '.DS_Store' -delete
chmod -R go-w "$BUILD/payload"; find "$BUILD/payload" -type f -exec chmod 644 {} \;; find "$BUILD/payload" -type d -exec chmod 755 {} \;
xattr -cr "$BUILD/payload" 2>/dev/null || true

pkgbuild --root "$BUILD/payload" --identifier "$PKGID" --version "$VERSION" \
  --scripts "$HERE/scripts" --install-location "/" --ownership recommended \
  "$BUILD/component.pkg"

cat > "$BUILD/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Bangla Keyboard Layout for Mac $VERSION</title>
  <welcome file="welcome.html"/>
  <volume-check><allowed-os-versions><os-version min="11.0"/></allowed-os-versions></volume-check>
  <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <choices-outline><line choice="default"/></choices-outline>
  <choice id="default" title="Bangla Keyboard"><pkg-ref id="$PKGID"/></choice>
  <pkg-ref id="$PKGID" version="$VERSION">component.pkg</pkg-ref>
</installer-gui-script>
XML

productbuild --distribution "$BUILD/distribution.xml" --resources "$HERE/resources" \
  --package-path "$BUILD" "$DIST/Bangla Keyboard.pkg"

# DMG
rm -rf "$BUILD/dmg"; mkdir -p "$BUILD/dmg"
cp "$DIST/Bangla Keyboard.pkg" "$BUILD/dmg/"
cp "$HERE/Bangla Keyboard Setup.command" "$BUILD/dmg/"; chmod 755 "$BUILD/dmg/Bangla Keyboard Setup.command"
cp "$ROOT/README.md" "$BUILD/dmg/Read Me.txt" 2>/dev/null || true
find "$BUILD/dmg" -name '._*' -delete; xattr -cr "$BUILD/dmg" 2>/dev/null || true
hdiutil create -volname "Bangla Keyboard" -srcfolder "$BUILD/dmg" -ov -format UDZO "$DIST/Bangla Keyboard.dmg" >/dev/null
rm -rf "$BUILD"
echo "Built:"; ls -lh "$DIST"
