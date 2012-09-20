#!/bin/sh


# Check whether DEVELOPER_SDK_DIR is set
if [ "$DEVELOPER_SDK_DIR"x = "x" ]; then
    DEVELOPER_SDK_DIR=/Developer/SDKs
    echo "Set DEVELOPER_SDK_DIR to $DEVELOPER_SDK_DIR"
else
    echo "DEVELOPER_SDK_DIR is $DEVELOPER_SDK_DIR"
fi


#################### CONFIGURE SYSTEM ################
# Check if configure script has to be run
echo "Checking configure options: $CONFIGURE_OPTIONS"

# collect current configuration (CONFIGURE_OPTIONS and COMPILE_DEFS_xyz)
OPTIONS_FILE_CUR=$BUILD_DIR/configure_options
echo "" > "$OPTIONS_FILE_CUR"
for ARCH in $ARCHS ; do
  COMPILE_DEFS=$(eval echo $(echo \$COMPILE_DEFS_$ARCH))
  echo "$COMPILE_DEFS" >> "$OPTIONS_FILE_CUR"
done
echo "$CONFIGURE_OPTIONS" >> "$OPTIONS_FILE_CUR"
echo `ls -lT "$SOURCE_DIR/../configure.ac"` >> "$OPTIONS_FILE_CUR"
FILE_CONTENT_CUR=`cat "$OPTIONS_FILE_CUR" 2>/dev/null`

# collect content of previous script execution
OPTIONS_FILE_OLD=$DERIVED_FILES_DIR/configure_options
FILE_CONTENT_OLD=`cat "$OPTIONS_FILE_OLD" 2>/dev/null`

if [ \( -f "$OPTIONS_FILE_OLD" \) -a \( "$FILE_CONTENT_OLD" == "$FILE_CONTENT_CUR" \) ]; then
  echo "Configuration still up-to-date. Skipping reconfiguration."	
  rsync -pogt "$DERIVED_FILES_DIR/"config*.h "$BUILD_DIR/" || ( rm "$OPTIONS_FILE" ; exit 1 )
  exit 0
fi

echo "Running configure script for the following architectures $ARCHS"

# generate a fresh options file
OPTIONS_FILE="$OPTIONS_FILE_OLD"
echo "" > "$OPTIONS_FILE"

# Check if PROJECT_DIR variable is set (Xcode 2.x)
if [ -z "$PROJECT_DIR" ]; then
  echo '$PROJECT_DIR WAS NOT DECLARED. SETTING VARIABLE:'
  old_dir=$PWD
  cd "$BUILD_DIR/.."
  export PROJECT_DIR="$PWD"
  cd $old_dir
  echo "	\$PROJECT_DIR = $PROJECT_DIR"
  echo
fi

# Make sure makedepend can be found
export PATH="$PATH:/usr/X11R6/bin"

# Make sure autoconf can be found
( autoconf --version ) > /dev/null 2>&1 || {
  echo "autoconf not found: appending /opt search locations to PATH (e.g. if MacPorts is used)"
  export PATH="$PATH:/opt/local/bin:/opt/local/sbin"
}


# Make sure SDL.m4 can be found
export ACLOCAL_FLAGS="-I $PROJECT_DIR/../darwin -I /usr/X11/share/aclocal"

# Make sure SDL.framework can be found
export LDFLAGS="-F$PROJECT_DIR"
export DYLD_FRAMEWORK_PATH=$PROJECT_DIR

# Make sure MPFR is found
export LDFLAGS="$LDFLAGS -L/opt/local/lib"

if [ ! -d "$DERIVED_FILES_DIR" ]; then
  echo "Creating $DERIVED_FILES_DIR"
  mkdir -p "$DERIVED_FILES_DIR"
fi

cd "$SOURCE_DIR/.."

# Remove old files
rm -rf Makefile autom4te.cache aclocal.m4 config.h config.log 2>/dev/null

# Generate configure script
if [ -f autogen.sh ]; then
  export NO_CONFIGURE=yes
  echo "Calling autogen.sh to prepare configure script"
  ./autogen.sh || exit 1
fi

# Configure system for all build architectures
echo "" > "$DERIVED_FILES_DIR/config.h"
for ARCH in $ARCHS ; do
  CPU_TYPE=$(eval echo $(echo \$CPU_TYPE_$ARCH))
  COMPILE_DEFS=$(eval echo $(echo \$COMPILE_DEFS_$ARCH))
  echo ; echo "Running configure for architecture $ARCH / $CPU_TYPE"
  echo "COMPILE_DEFS=$COMPILE_DEFS"
  echo $PWD

  ./configure $CONFIGURE_OPTIONS --host=$ARCH-apple-$OSTYPE || exit 1

  if [ "$ARCH" = "ppc" -a "$SDK_NAME" = "macosx10.3.9" ]; then
    # 10.3.9 compatibility:
	echo "Special handling of PPC 10.3.9 build"
    cat config.h | sed 's/#define HAVE_SYS_STATVFS_H 1/\/* #undef HAVE_SYS_STATVFS_H *\//' > config_$ARCH.h
    rm config.h
  else
    mv config.h config_$ARCH.h
  fi
  cat  >> "$DERIVED_FILES_DIR/config.h" << EOF
#ifdef $CPU_TYPE
#include "config_$ARCH.h"
#endif

EOF

  # Check whether the COMPILE_DEFS flag is still up-to-date
  DEFS="`sed -n -e 's/DEFS = \(.*\)/ \1/p' Makefile | sed 's/ -D/ /g'`"
  echo $COMPILE_DEFS | sed 's/ /\
/g' | sort > "$DERIVED_FILES_DIR/defs_xcode.txt"

  echo $DEFS | sed 's/ /\
/g' | sort > "$DERIVED_FILES_DIR/defs_make.txt"

  diff -u "$DERIVED_FILES_DIR/defs_xcode.txt" "$DERIVED_FILES_DIR/defs_make.txt" > "$DERIVED_FILES_DIR/defs_delta.txt"
  if [ `wc -l < "$DERIVED_FILES_DIR/defs_delta.txt"` -gt 0 ]; then
    echo "error: Invalid COMPILE_DEFS_$ARCH set in the target build setting."
    echo "Please add the following flags:"
    grep -e "^+[^+]" "$DERIVED_FILES_DIR/defs_delta.txt" | sed 's/+//'
    echo 
    echo "Please remove the following flags:"
    grep -e "^-[^-]" "$DERIVED_FILES_DIR/defs_delta.txt" | sed 's/-//'
    echo
    echo "Reminder: the following definitions are active for this target & architecture ($ARCH)"
    set | grep "COMPILE_DEF"
    exit 2
  fi

  mv config_$ARCH.h "$DERIVED_FILES_DIR"
  mv Makefile "$DERIVED_FILES_DIR/Makefile_$ARCH"

  echo "$COMPILE_DEFS" >> "$OPTIONS_FILE"
done

# Remember configure options for next script execution
echo "$CONFIGURE_OPTIONS" >> "$OPTIONS_FILE"

# Remember configure.ac timestamp
echo `ls -lT "$SOURCE_DIR/../configure.ac"` >> "$OPTIONS_FILE"

echo "Configuration generated:"
cp "$DERIVED_FILES_DIR/"config*.h "$BUILD_DIR/" || exit 1

exit 0