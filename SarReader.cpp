/* -*- C++ -*-
 *
 *  SarReader.cpp - Reader from a SAR archive
 *
 *  Copyright (c) 2001-2006 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "SarReader.h"
#define WRITE_LENGTH 4096

SarReader::SarReader( char *path, const unsigned char *key_table )
        :DirectReader( path, key_table )
{
    root_archive_info = last_archive_info = &archive_info;
    num_of_sar_archives = 0;
}

SarReader::~SarReader()
{
    close();
}

int SarReader::open( char *name, int archive_type )
{
    ArchiveInfo* info = new ArchiveInfo();

    if ( (info->file_handle = fopen( name, "rb" ) ) == NULL ){
        delete info;
        return -1;
    }

    info->file_name = new char[strlen(name)+1];
    memcpy(info->file_name, name, strlen(name)+1);
    
    readArchive( info );

    last_archive_info->next = info;
    last_archive_info = last_archive_info->next;
    num_of_sar_archives++;

    return 0;
}

int SarReader::readArchive( struct ArchiveInfo *ai, int archive_type )
{
    unsigned int i=0;
    
    /* Read header */
    if ( archive_type == ARCHIVE_TYPE_NS2 ){
        i = readChar( ai->file_handle ); // FIXME: for what ?
    }
    if ( archive_type == ARCHIVE_TYPE_NS3 ){
        i = readChar( ai->file_handle ); // FIXME: for what ?
        i = readChar( ai->file_handle ); // FIXME: for what ?
    }
    ai->num_of_files = readShort( ai->file_handle );
    ai->fi_list = new struct FileInfo[ ai->num_of_files ];
    
    ai->base_offset = readLong( ai->file_handle );
    if ( archive_type == ARCHIVE_TYPE_NS2 )
        ai->base_offset++;
    if ( archive_type == ARCHIVE_TYPE_NS3 )
        ai->base_offset += 2;
    
    for ( i=0 ; i<ai->num_of_files ; i++ ){
        unsigned char ch;
        int count = 0;

        while( (ch = key_table[fgetc( ai->file_handle )] ) ){
            if ( 'a' <= ch && ch <= 'z' ) ch += 'A' - 'a';
            ai->fi_list[i].name[count++] = ch;
        }
        ai->fi_list[i].name[count] = ch;

        if ( archive_type >= ARCHIVE_TYPE_NSA )
            ai->fi_list[i].compression_type = readChar( ai->file_handle );
        else
            ai->fi_list[i].compression_type = NO_COMPRESSION;
        ai->fi_list[i].offset = readLong( ai->file_handle ) + ai->base_offset;
        ai->fi_list[i].length = readLong( ai->file_handle );

        if ( archive_type >= ARCHIVE_TYPE_NSA ){
            ai->fi_list[i].original_length = readLong( ai->file_handle );
        }
        else{
            ai->fi_list[i].original_length = ai->fi_list[i].length;
        }

        /* Registered Plugin check */
        if ( ai->fi_list[i].compression_type == NO_COMPRESSION )
            ai->fi_list[i].compression_type = getRegisteredCompressionType( ai->fi_list[i].name );

        if ( ai->fi_list[i].compression_type == NBZ_COMPRESSION ||
             ai->fi_list[i].compression_type == SPB_COMPRESSION ){
            ai->fi_list[i].original_length = getDecompressedFileLength( ai->fi_list[i].compression_type, ai->file_handle, ai->fi_list[i].offset );
        }
    }
    
    return 0;
}

int SarReader::writeHeaderSub( ArchiveInfo *ai, FILE *fp, int archive_type )
{
    unsigned int i, j;

    fseek( fp, 0L, SEEK_SET );
    if ( archive_type == ARCHIVE_TYPE_NS2 ) fputc( 0, fp );
    if ( archive_type == ARCHIVE_TYPE_NS3 ){
        fputc( 0, fp );
        fputc( 0, fp );
    }
    writeShort( fp, ai->num_of_files  );
    if ( archive_type == ARCHIVE_TYPE_NS2 )
        writeLong( fp, ai->base_offset-1 );
    else if ( archive_type == ARCHIVE_TYPE_NS3 )
        writeLong( fp, ai->base_offset-2 );
    else
        writeLong( fp, ai->base_offset );
    
    for ( i=0 ; i<ai->num_of_files ; i++ ){

        for ( j=0 ; ai->fi_list[i].name[j] ; j++ )
            fputc( ai->fi_list[i].name[j], fp );
        fputc( ai->fi_list[i].name[j], fp );
        
        if ( archive_type >= ARCHIVE_TYPE_NSA )
            writeChar( fp, ai->fi_list[i].compression_type );

        writeLong( fp, ai->fi_list[i].offset  - ai->base_offset );
        writeLong( fp, ai->fi_list[i].length );

        if ( archive_type >= ARCHIVE_TYPE_NSA ){
            writeLong( fp, ai->fi_list[i].original_length );
        }
    }

    return 0;
}

int SarReader::writeHeader( FILE *fp )
{
    ArchiveInfo *ai = archive_info.next;
    return writeHeaderSub( ai, fp );
}

size_t SarReader::putFileSub( ArchiveInfo *ai, FILE *fp, int no, size_t offset, size_t length, size_t original_length, int compression_type, bool modified_flag, unsigned char *buffer )
{
    ai->fi_list[no].compression_type = compression_type;
    ai->fi_list[no].length = length;
    ai->fi_list[no].original_length = original_length;

    fseek( fp, offset, SEEK_SET );
    if ( modified_flag ){
        if ( ai->fi_list[no].compression_type == NBZ_COMPRESSION ){
            writeLong( fp, ai->fi_list[no].original_length );
            fseek( ai->file_handle, ai->fi_list[no].offset+2, SEEK_SET );
            if ( readChar( ai->file_handle ) != 'B' || readChar( ai->file_handle ) != 'Z' ){ // in case the original is not compressed in NBZ
                ai->fi_list[no].length = encodeNBZ( fp, length, buffer ) + 4;
                ai->fi_list[no].offset = offset;
                return ai->fi_list[no].length;
            }
        }
        else{
            ai->fi_list[no].compression_type = NO_COMPRESSION;
        }
    }
    else{
        fseek( ai->file_handle, ai->fi_list[no].offset, SEEK_SET );
        fread( buffer, 1, ai->fi_list[no].length, ai->file_handle );
    }

    size_t len = ai->fi_list[no].length, c;
    while( len > 0 ){
        if ( len > WRITE_LENGTH ) c = WRITE_LENGTH;
        else                      c = len;
        len -= c;
        fwrite( buffer, 1, c, fp );
        buffer += c;
    }

    ai->fi_list[no].offset = offset;
    
    return ai->fi_list[no].length;
}

size_t SarReader::putFile( FILE *fp, int no, size_t offset, size_t length, size_t original_length, bool modified_flag, unsigned char *buffer )
{
    ArchiveInfo *ai = archive_info.next;
    return putFileSub( ai, fp, no, offset, length, original_length, ai->fi_list[no].compression_type, modified_flag, buffer );
}

int SarReader::close()
{
    ArchiveInfo *info = archive_info.next;
    
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        if ( info->file_handle ){
            fclose( info->file_handle );
            delete[] info->file_name;
            delete[] info->fi_list;
        }
        last_archive_info = info;
        info = info->next;
        delete last_archive_info;
    }
    return 0;
}

char *SarReader::getArchiveName() const
{
    return "sar";
}

int SarReader::getNumFiles(){
    ArchiveInfo *info = archive_info.next;
    int num = 0;
    
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        num += info->num_of_files;
        info = info->next;
    }
    
    return num;
}

int SarReader::getIndexFromFile( ArchiveInfo *ai, const char *file_name )
{
    unsigned int i, len;

    len = strlen( file_name );
    if ( len > MAX_FILE_NAME_LENGTH ) len = MAX_FILE_NAME_LENGTH;
    memcpy( capital_name, file_name, len );
    capital_name[ len ] = '\0';

    for ( i=0 ; i<len ; i++ ){
        if ( 'a' <= capital_name[i] && capital_name[i] <= 'z' ) capital_name[i] += 'A' - 'a';
        else if ( capital_name[i] == '/' ) capital_name[i] = '\\';
    }
    for ( i=0 ; i<ai->num_of_files ; i++ ){
        if ( !strcmp( capital_name, ai->fi_list[i].name ) ) break;
    }

    return i;
}

size_t SarReader::getFileLength( const char *file_name )
{
    size_t ret;
    if ( ( ret = DirectReader::getFileLength( file_name ) ) ) return ret;

    ArchiveInfo *info = archive_info.next;
    unsigned int j = 0;
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        j = getIndexFromFile( info, file_name );
        if ( j != info->num_of_files ) break;
        info = info->next;
    }
    if ( !info ) return 0;
    
    if ( info->fi_list[j].compression_type == NO_COMPRESSION ){
        int type = getRegisteredCompressionType( file_name );
        if ( type == NBZ_COMPRESSION || type == SPB_COMPRESSION )
            return getDecompressedFileLength( type, info->file_handle, info->fi_list[j].offset );
    }
    
    return info->fi_list[j].original_length;
}

size_t SarReader::getFileSub( ArchiveInfo *ai, const char *file_name, unsigned char *buf )
{
    unsigned int i = getIndexFromFile( ai, file_name );
    if ( i == ai->num_of_files ) return 0;

    int type = ai->fi_list[i].compression_type;
    if ( type == NO_COMPRESSION ) type = getRegisteredCompressionType( file_name );

    if      ( type == NBZ_COMPRESSION ){
        return decodeNBZ( ai->file_handle, ai->fi_list[i].offset, buf );
    }
    else if ( type == LZSS_COMPRESSION ){
        return decodeLZSS( ai, i, buf );
    }
    else if ( type == SPB_COMPRESSION ){
        return decodeSPB( ai->file_handle, ai->fi_list[i].offset, buf );
    }

    fseek( ai->file_handle, ai->fi_list[i].offset, SEEK_SET );
    size_t ret = fread( buf, 1, ai->fi_list[i].length, ai->file_handle );
    for (size_t j=0 ; j<ret ; j++) buf[j] = key_table[buf[j]];
    return ret;
}

size_t SarReader::getFile( const char *file_name, unsigned char *buf, int *location )
{
    size_t ret;
    if ( ( ret = DirectReader::getFile( file_name, buf, location ) ) ) return ret;

    ArchiveInfo *info = archive_info.next;
    size_t j = 0;
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        if ( (j = getFileSub( info, file_name, buf )) > 0 ) break;
        info = info->next;
    }
    if ( location ) *location = ARCHIVE_TYPE_SAR;
    
    return j;
}

struct SarReader::FileInfo SarReader::getFileByIndex( unsigned int index )
{
    ArchiveInfo *info = archive_info.next;
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        if ( index < info->num_of_files ) return info->fi_list[index];
        index -= info->num_of_files;
        info = info->next;
    }
    fprintf( stderr, "SarReader::getFileByIndex  Index %d is out of range\n", index );

    return archive_info.fi_list[index];
}
