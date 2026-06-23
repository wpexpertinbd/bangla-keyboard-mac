#!/bin/bash
# Bangla Keyboard — smart Setup (Install / Reinstall / Uninstall)
# Double-click to run. Detects whether the keyboard is already installed and
# offers the right action. by BiswasHost — https://www.biswashost.com

DIR="$(cd "$(dirname "$0")" && pwd)"
PKG="$DIR/Bangla Keyboard.pkg"
KL="/Library/Keyboard Layouts"
LAYOUT="$KL/Bangla Unicode.keylayout"
TITLE="Bangla Keyboard Setup"

FONTS=(AdorshoLipi_20-07-2007.ttf AponaLohit.ttf Bangla.ttf BenSen.ttf \
BenSenHandwriting.ttf Lohit_14-04-2007.ttf Mukti_1.99_PR.ttf Siyamrupali.ttf \
SolaimanLipi.ttf akaashnormal.ttf kalpurush.ttf mitra.ttf muktinarrow.ttf sagarnormal.ttf)

dialog() { # $1=text  $2=buttons(as AppleScript list)  $3=default
  osascript -e "button returned of (display dialog \"$1\" buttons $2 default button \"$3\" with title \"$TITLE\" with icon note)" 2>/dev/null
}
admin() { osascript -e "do shell script \"$1\" with administrator privileges" 2>/dev/null; }
info()  { osascript -e "display dialog \"$1\" buttons {\"OK\"} default button \"OK\" with title \"$TITLE\" with icon note" >/dev/null 2>&1; }

run_install() {
  if [ ! -f "$PKG" ]; then info "Installer package not found next to this script."; exit 1; fi
  if admin "installer -pkg '$PKG' -target /"; then
    info "Installed ✅\n\nNow:\n1) LOG OUT and back in (or restart).\n2) System Settings → Keyboard → Text Input → Edit… → + → Bangla → add the layouts.\n\n(macOS caches keyboard layouts, so the log-out is required.)"
  else
    info "Install was cancelled or failed."
  fi
}

run_uninstall() {
  local cmd="rm -f '$KL/Bangla Unicode.keylayout' '$KL/Bangla Unicode.icns' '$KL/Bangla Classic.keylayout' '$KL/Bangla Classic.icns'"
  for f in "${FONTS[@]}"; do cmd="$cmd '/Library/Fonts/$f'"; done
  if admin "$cmd"; then
    info "Uninstalled ✅\n\nThe layout files and the bundled fonts were removed.\nIf the layouts still show in the menu, remove them in\nSystem Settings → Keyboard → Text Input → Edit…, then log out/in."
  else
    info "Uninstall was cancelled or failed."
  fi
}

if [ -f "$LAYOUT" ]; then
  choice=$(dialog "Bangla Keyboard is already installed.\n\nWhat would you like to do?" '{"Cancel","Uninstall","Reinstall"}' "Reinstall")
  case "$choice" in
    Reinstall) run_install ;;
    Uninstall) run_uninstall ;;
    *) exit 0 ;;
  esac
else
  choice=$(dialog "Bangla Keyboard is not installed.\n\nInstall it now?" '{"Cancel","Install"}' "Install")
  case "$choice" in
    Install) run_install ;;
    *) exit 0 ;;
  esac
fi
