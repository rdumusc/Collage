
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *               2009-2010, Cedric Stalder <cedric.stalder@gmail.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef CO_DATAISTREAM_H
#define CO_DATAISTREAM_H

#include <co/api.h>
#include <co/array.h> // used inline
#include <co/types.h>

#include <vector>

namespace co
{
namespace detail { class DataIStream; }

    /** A std::istream-like input data stream for binary data. */
    class DataIStream
    {
    public:
        /** @name Internal */
        //@{
        /** @internal @return the number of remaining buffers. */
        virtual size_t nRemainingBuffers() const = 0;

        virtual uint128_t getVersion() const = 0; //!< @internal
        virtual void reset() { _reset(); } //!< @internal
        void setSwapping( const bool onOff ); //!< @internal enable endian swap
        CO_API bool isSwapping() const; //!< @internal
        //@}

        /** @name Data input */
        //@{
        /** Read a plain data item. @version 1.0 */
        template< class T > DataIStream& operator >> ( T& value )
            { _read( &value, sizeof( value )); _swap( value ); return *this; }

        /** Read a C array. @version 1.0 */
        template< class T > DataIStream& operator >> ( Array< T > array )
            {
                _read( array.data, array.getNumBytes( ));
                _swap( array );
                return *this;
            }

        /** Byte-swap a plain data item. @version 1.0 */
        template< class T > static void swap( T& value )
            { lunchbox::byteswap( value ); }

        /** Read a lunchbox::Buffer. @version 1.0 */
        template< class T > DataIStream& operator >> ( lunchbox::Buffer< T >& );

        /** Read a std::vector of serializable items. @version 1.0 */
        template< class T > DataIStream& operator >> ( std::vector< T >& );

        /** @internal
         * Deserialize child objects.
         *
         * Existing children are synced to the new version. New children are
         * created by calling the <code>void create( C** child )</code> method
         * on the object, and registered or mapped to the object's
         * session. Removed children are released by calling the <code>void
         * release( C* )</code> method on the object. The resulting child vector
         * is created in result. The old and result vector can be the same
         * object, the result vector is cleared and rebuild completely.
         */
        template< typename O, typename C >
        void deserializeChildren( O* object, const std::vector< C* >& old,
                                  std::vector< C* >& result );

        /**
         * Get the pointer to the remaining data in the current buffer.
         *
         * The usage of this method is discouraged, no endian conversion or
         * bounds checking is performed by the DataIStream on the returned raw
         * pointer.
         *
         * The buffer is advanced by the given size. If not enough data is
         * present, 0 is returned and the buffer is unchanged.
         *
         * The data written to the DataOStream by the sender is bucketized, it
         * is sent in multiple blocks. The remaining buffer and its size points
         * into one of the buffers, i.e., not all the data sent is returned by
         * this function. However, a write operation on the other end is never
         * segmented, that is, if the application writes n bytes to the
         * DataOStream, a symmetric read from the DataIStream has at least n
         * bytes available.
         *
         * @param size the number of bytes to advance the buffer
         * @version 1.0
         */
        CO_API const void* getRemainingBuffer( const uint64_t size );

        /**
         * @return the size of the remaining data in the current buffer.
         * @version 1.0
         */
        CO_API uint64_t getRemainingBufferSize();

        /** @return true if not all data has been read. @version 1.0 */
        bool hasData() { return _checkBuffer(); }

        /** @return the provider of the istream. */
        CO_API virtual NodePtr getMaster() = 0;
        //@}

    protected:
        /** @name Internal */
        //@{
        CO_API DataIStream();
        DataIStream( const DataIStream& );
        CO_API virtual ~DataIStream();
        //@}

        virtual bool getNextBuffer( uint32_t& compressor, uint32_t& nChunks,
                                    const void** chunkData, uint64_t& size )=0;
    private:
        detail::DataIStream* const _impl;

        /** Read a number of bytes from the stream into a buffer. */
        CO_API void _read( void* data, uint64_t size );

        /**
         * Check that the current buffer has data left, get the next buffer is
         * necessary, return false if no data is left.
         */
        CO_API bool _checkBuffer();
        CO_API void _reset();

        const uint8_t* _decompress( const void* data, const uint32_t name,
                                    const uint32_t nChunks,
                                    const uint64_t dataSize );

        /** Read a vector of trivial data. */
        template< class T >
        DataIStream& _readFlatVector ( std::vector< T >& value )
        {
            uint64_t nElems = 0;
            (*this) >> nElems;
            LBASSERTINFO( nElems < LB_BIT48,
                  "Out-of-sync co::DataIStream: " << nElems << " elements?" );
            value.resize( size_t( nElems ));
            if( nElems > 0 )
                (*this) >> Array< T >( &value.front(), nElems );
            return *this;
        }

        /** Byte-swap a plain data item. @version 1.0 */
        template< class T > void _swap( T& value ) const 
            { if( isSwapping( )) swap( value ); }

        /** Byte-swap a C array. @version 1.0 */
        template< class T > void _swap( Array< T > array ) const
            {
                if( !isSwapping( ))
                    return;
#pragma omp parallel for
                for( ssize_t i = 0; i < ssize_t( array.num ); ++i )
                    swap( array.data[i] );
            }
    };
}

#include <co/object.h>
#include <co/objectVersion.h>

namespace co
{
    /** @name Specialized input operators */
    //@{
    /** Read a std::string. */
    template<>
    inline DataIStream& DataIStream::operator >> ( std::string& str )
    {
        uint64_t nElems = 0;
        (*this) >> nElems;
        LBASSERTINFO( nElems <= getRemainingBufferSize(),
                      nElems << " > " << getRemainingBufferSize( ));
        if( nElems == 0 )
            str.clear();
        else
            str.assign( static_cast< const char* >( getRemainingBuffer(nElems)),
                        size_t( nElems ));
        return *this;
    }

    /** Deserialize an object (id+version). */
    template<> inline DataIStream& DataIStream::operator >> ( Object*& object )
    {
        ObjectVersion data;
        (*this) >> data;
        LBASSERT( object->getID() == data.identifier );
        object->sync( data.version );
        return *this;
    }

/** @cond IGNORE */
    template< class T > inline DataIStream&
    DataIStream::operator >> ( lunchbox::Buffer< T >& buffer )
    {
        uint64_t nElems = 0;
        (*this) >> nElems;
        buffer.resize( nElems );
        return (*this) >> Array< T >( buffer.getData(), buffer.getNumBytes( ));
    }


    template< class T > inline DataIStream&
    DataIStream::operator >> ( std::vector< T >& value )
    {
        uint64_t nElems = 0;
        (*this) >> nElems;
        value.resize( nElems );
        for( uint64_t i = 0; i < nElems; i++ )
            (*this) >> value[i];

        return *this;
    }

    namespace
    {
    class ObjectFinder
    {
    public:
        ObjectFinder( const UUID& id ) : _id( id ) {}
        bool operator()( co::Object* candidate )
            { return candidate->getID() == _id; }

    private:
        const UUID _id;
    };
    }

    template<> inline void DataIStream::_swap( Array< void > ) const { /*NOP*/ }

    template< typename O, typename C > inline void
    DataIStream::deserializeChildren( O* object, const std::vector< C* >& old_,
                                      std::vector< C* >& result )
    {
        ObjectVersions versions;
        (*this) >> versions;
        std::vector< C* > old = old_;

        // rebuild vector from serialized list
        result.clear();
        for( ObjectVersions::const_iterator i = versions.begin();
             i != versions.end(); ++i )
        {
            const ObjectVersion& version = *i;

            if( version.identifier == UUID::ZERO )
            {
                result.push_back( 0 );
                continue;
            }

            typename std::vector< C* >::iterator j =
                stde::find_if( old, ObjectFinder( version.identifier ));

            if( j == old.end( )) // previously unknown child
            {
                C* child = 0;
                object->create( &child );
                LocalNodePtr localNode = object->getLocalNode();
                LBASSERT( child );
                LBASSERT( !object->isMaster( ));

                LBCHECK( localNode->mapObject( child, version ));
                result.push_back( child );
            }
            else
            {
                C* child = *j;
                old.erase( j );
                if( object->isMaster( ))
                    child->sync( VERSION_HEAD );
                else
                    child->sync( version.version );

                result.push_back( child );
            }
        }

        while( !old.empty( )) // removed children
        {
            C* child = old.back();
            old.pop_back();
            if( !child )
                continue;

            if( child->isAttached() && !child->isMaster( ))
            {
                LocalNodePtr localNode = object->getLocalNode();
                localNode->unmapObject( child );
            }
            object->release( child );
        }
    }
/** @endcond */

    /** Optimized specialization to read a std::vector of uint8_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< uint8_t >& value )
    { return _readFlatVector( value );}

    /** Optimized specialization to read a std::vector of uint16_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< uint16_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of int16_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< int16_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of uint32_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< uint32_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of int32_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< int32_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of uint64_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< uint64_t>& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of int64_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< int64_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of float. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< float >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of double. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< double >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of ObjectVersion. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< ObjectVersion >& value )
    { return _readFlatVector( value ); }
    //@}
}

#endif //CO_DATAISTREAM_H