#!/bin/sh

assembly=$1
srcdir=./sources

FILE=$srcdir/$assembly.dll.config.in
echo "Generating $FILE"
echo "<configuration>" > $FILE
echo "  <dllmap dll=\"monoskel\" target=\"monoskel@LIB_PREFIX@.1@LIB_SUFFIX@\"/>" >> $FILE
echo "</configuration>" >> $FILE

FILE=$srcdir/$assembly.metadata
echo "Generating $FILE"
echo "<?xml version=\"1.0\"?>" > $FILE
echo "<metadata>" >> $FILE
echo "</metadata>" >> $FILE

FILE=$srcdir/$assembly.pc.in
echo "Generating $FILE"
echo "prefix=@prefix@" >> $FILE
echo "exec_prefix=${prefix}" >> $FILE
echo "libdir=${exec_prefix}/lib" >> $FILE
echo "" >> $FILE
echo "Name: @ASSEMBLY_TITLE@" >> $FILE
echo "Description: @ASSEMBLY_DESCRIPTION@" >> $FILE
echo "Version: @VERSION@" >> $FILE
echo "Requires: gtk-sharp-2.0" >> $FILE
echo "Libs: -r:${prefix}/lib/mono/@ASSEMBLY_NAME@/@ASSEMBLY_NAME@.dll" >> $FILE

echo "Generating strong name key"
sn -q -k $srcdir/$assembly.snk

FILE=$srcdir/$assembly-sources.xml
echo "Generating $FILE"
echo "<?xml version=\"1.0\"?>" > $FILE
echo "<gapi-parser-input>" >> $FILE
echo "  <api filename=\"$assembly-api.raw\">" >> $FILE
echo "    <library name=\"monoskel-1.0\">" >> $FILE
echo "      <namespace name=\"MonoSkel\">" >> $FILE
echo "        <dir>/use/include/monoskel</dir>" >> $FILE
echo "      </namespace>" >> $FILE
echo "    </library>" >> $FILE
echo "  </api>" >> $FILE
echo "</gapi-parser-input>" >> $FILE

# change package name in configure.in
#sed -i -e "s,monoskel-sharp,$assembly," configure.in
sed -i -e "s,AM_INIT_AUTOMAKE(.*\,,AM_INIT_AUTOMAKE($assembly\,," configure.in


