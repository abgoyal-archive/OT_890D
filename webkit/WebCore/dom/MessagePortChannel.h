

#ifndef MessagePortChannel_h
#define MessagePortChannel_h

#include "PlatformString.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class MessagePort;
    class PlatformMessagePortChannel;
    class ScriptExecutionContext;
    class String;

    // MessagePortChannel is a platform-independent interface to the remote side of a message channel.
    // It acts as a wrapper around the platform-dependent PlatformMessagePortChannel implementation which ensures that the platform-dependent close() method is invoked before destruction.
    class MessagePortChannel : public Noncopyable {
    public:
        static void createChannel(PassRefPtr<MessagePort>, PassRefPtr<MessagePort>);

        // Creates a new wrapper for the passed channel.
        static PassOwnPtr<MessagePortChannel> create(PassRefPtr<PlatformMessagePortChannel>);

        // Entangles the channel with a port (called when a port has been cloned, after the clone has been marshalled to its new owning thread and is ready to receive messages).
        // Returns false if the entanglement failed because the port was closed.
        bool entangleIfOpen(MessagePort*);

        // Disentangles the channel from a given port so it no longer forwards messages to the port. Called when the port is being cloned and no new owning thread has yet been established.
        void disentangle();

        // Closes the port (ensures that no further messages can be added to either queue).
        void close();

        // Used by MessagePort.postMessage() to prevent callers from passing a port's own entangled port.
        bool isConnectedTo(MessagePort*);

        // Returns true if the proxy currently contains messages for this port.
        bool hasPendingActivity();

        class EventData {
        public:
            static PassOwnPtr<EventData> create(const String&, PassOwnPtr<MessagePortChannel>);

            const String& message() { return m_message; }
            PassOwnPtr<MessagePortChannel> channel() { return m_channel.release(); }

        private:
            EventData(const String& message, PassOwnPtr<MessagePortChannel>);
            String m_message;
            OwnPtr<MessagePortChannel> m_channel;
        };

        // Sends a message and optional cloned port to the remote port.
        void postMessageToRemote(PassOwnPtr<EventData>);

        // Extracts a message from the message queue for this port.
        bool tryGetMessageFromRemote(OwnPtr<EventData>&);

        // Returns the entangled port if run by the same thread (see MessagePort::locallyEntangledPort() for more details).
        MessagePort* locallyEntangledPort(const ScriptExecutionContext*);

        ~MessagePortChannel();

        PlatformMessagePortChannel* channel() const { return m_channel.get(); }

    private:
        MessagePortChannel(PassRefPtr<PlatformMessagePortChannel>);
        RefPtr<PlatformMessagePortChannel> m_channel;
    };

} // namespace WebCore

#endif // MessagePortChannel_h
