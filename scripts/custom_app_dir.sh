#!/bin/bash -e

this_dir="\$(readlink -f "\$(dirname "\$0")")"
export XDG_DATA_DIRS="\${this_dir}/usr/share:\${XDG_DATA_DIRS}:/usr/share:/usr/local/share"

# Force set openssl config directory to an invalid directory to fallback to use default openssl config.
# This can avoid some distributions (mainly Fedora) having some strange patches or configurations
# for openssl that make the libssl in Appimage bundle unavailable.
export OPENSSL_CONF="\${this_dir}"

# Find the system certificates location
# https://gitlab.com/probono/platformissues/blob/master/README.md#certificates
possible_locations=(
  "/etc/ssl/certs/ca-certificates.crt"                # Debian/Ubuntu/Gentoo etc.
  "/etc/pki/tls/certs/ca-bundle.crt"                  # Fedora/RHEL
  "/etc/ssl/ca-bundle.pem"                            # OpenSUSE
  "/etc/pki/tls/cacert.pem"                           # OpenELEC
  "/etc/ssl/certs"                                    # SLES10/SLES11, https://golang.org/issue/12139
  "/usr/share/ca-certs/.prebuilt-store/"              # Clear Linux OS; https://github.com/knapsu/plex-media-player-appimage/issues/17#issuecomment-437710032
  "/system/etc/security/cacerts"                      # Android
  "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem" # CentOS/RHEL 7
  "/etc/ssl/cert.pem"                                 # Alpine Linux
)

for location in "\${possible_locations[@]}"; do
  if [ -r "\${location}" ]; then
    export SSL_CERT_FILE="\${location}"
    break
  fi
done

exec "\${this_dir}/usr/bin/quentier" "\$@"

