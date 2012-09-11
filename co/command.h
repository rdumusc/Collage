
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_COMMAND_H
#define CO_COMMAND_H

#include <co/api.h>
#include <co/commands.h>        // for enum CommandType
#include <co/dataIStream.h>     // base class


namespace co
{
namespace detail { class Command; }

    /**
     * A class managing received commands.
     *
     * This class is used by the LocalNode to pass received buffers to the
     * Dispatcher and ultimately command handler functions. It is not intended
     * to be instantiated by applications. The derivates of this Command have to
     * be instaniated by the application if the command type requires it. The
     * data retrieval is possible with the provided DataIStream methods or with
     * the templated get() function.
     */
    class Command : public DataIStream
    {
    public:
        CO_API Command(); //!< @internal

        CO_API Command( ConstBufferPtr buffer ); //!< @internal

        CO_API Command( const Command& rhs ); //!< @internal
        CO_API Command& operator = ( const Command& rhs ); //!< @internal

        CO_API void clear(); //!< @internal

        CO_API virtual ~Command(); //!< @internal

        /** @name Data Access */
        //@{
        /** @return the command type. @version 1.0 */
        CO_API uint32_t getType() const;

        /** @return the command. @version 1.0 */
        CO_API uint32_t getCommand() const;

        /** @internal @return the size of this command. @version 1.0 */
        CO_API uint64_t getSize() const;

        /** @return a value from the command. @version 1.0 */
        template< typename T > T get()
        {
            T value;
            *this >> value;
            return value;
        }

        /** @return the sending node proxy instance. @version 1.0 */
        CO_API NodePtr getNode() const;

        /** @return the receiving node. @version 1.0 */
        CO_API LocalNodePtr getLocalNode() const;

        /** @return true if the command has valid data. @version 1.0 */
        CO_API bool isValid() const;
        //@}

        /** @internal @name Dispatch command functions.. */
        //@{
        /** Change the command type for subsequent dispatching. */
        CO_API void setType( const CommandType type );

        /** Change the command for subsequent dispatching. */
        CO_API void setCommand( const uint32_t cmd );

        /** Set the function to which the command is dispatched. */
        void setDispatchFunction( const Dispatcher::Func& func );

        /** Invoke and clear the command function of a dispatched command. */
        CO_API bool operator()();
        //@}

    private:
        detail::Command* const _impl;

        friend CO_API std::ostream& operator << (std::ostream&, const Command&);

        /** @internal @name DataIStream functions. */
        //@{
        CO_API virtual size_t nRemainingBuffers() const;
        CO_API virtual uint128_t getVersion() const;
        CO_API virtual NodePtr getMaster();
        CO_API virtual bool getNextBuffer( uint32_t& compressor,
                                           uint32_t& nChunks,
                                           const void** chunkData,
                                           uint64_t& size );
        //@}

        void _skipHeader(); //!< @internal
    };

    CO_API std::ostream& operator << ( std::ostream& os, const Command& );
}
#endif // CO_COMMAND_H