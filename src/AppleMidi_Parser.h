BEGIN_APPLEMIDI_NAMESPACE

#include <midi_RingBuffer.h>
using namespace MIDI_NAMESPACE;

#include "AppleMidi_Util.h"

template<class UdpClass>
class Session;

template<class UdpClass>
class AppleMIDIParser
{
public:
	static int Parser(midi::RingBuffer<byte, BUFFER_MAX_SIZE>& buffer, Session<UdpClass>* session, const amPortType& portType)
	{
		//Serial.print("AppleMIDI_Parser::Parser received ");
		//Serial.print(buffer.getLength());
		//Serial.println(" bytes");

		uint16_t minimumLen = 4;
		if (buffer.getLength() < minimumLen)
			return 0;

		uint16_t i = 0;

		byte signature[2]; // Signature "Magic Value" for Apple network MIDI session establishment 
		signature[0] = buffer.peek(i++);
		signature[1] = buffer.peek(i++);
		if (0 != memcmp(signature, amSignature, sizeof(amSignature)))
		{
			//Serial.print("Wrong signature: 0x");
			//Serial.print(signature[0], HEX);
			//Serial.print(signature[1], HEX);
			//Serial.println(" was expecting 0xFFFF");

			return 0;
		}

		byte command[2]; // 16-bit command identifier (two ASCII characters, first in high 8 bits, second in low 8 bits)
		command[0] = buffer.peek(i++);
		command[1] = buffer.peek(i++);

		if (false)
		{
		}
#ifdef SLAVE
		else if (0 == memcmp(command, amInvitation, sizeof(amInvitation)))
		{
			//Serial.println("received Invitation");

			// minimum amount : 4 bytes for protocol version, 4 bytes for initiator token, 4 bytes for sender SSRC
			minimumLen += (4 + 4 + 4);
			if (buffer.getLength() < minimumLen) 
				return 0;

			// 2 (stored in network byte order (big-endian))
			byte protocolVersion[4];
			protocolVersion[0] = buffer.peek(i++);
			protocolVersion[1] = buffer.peek(i++);
			protocolVersion[2] = buffer.peek(i++);
			protocolVersion[3] = buffer.peek(i++);
			if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
			{
				//Serial.print("Wrong protocolVersion: 0x");
				//Serial.print(protocolVersion[0], HEX);
				//Serial.print(protocolVersion[1], HEX);
				//Serial.print(protocolVersion[2], HEX);
				//Serial.print(protocolVersion[3], HEX);
				//Serial.println(" was expecting 0x00000002");
				return 0;
			}

			AppleMIDI_Invitation invitation;

			// A random number generated by the session's initiator.
			invitation.initiatorToken = ntohl(buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++));
			// The sender's synchronization source identifier.
			invitation.ssrc = ntohl(buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++));

			//Serial.print("initiatorToken: 0x");
			//Serial.println(invitation.initiatorToken, HEX);
			//Serial.print("senderSSRC: 0x");
			//Serial.println(invitation.ssrc, HEX);

			uint16_t bi = 0;
			while (i < buffer.getLength() && buffer.peek(i) != 0x00 && bi <= SESSION_NAME_MAX_LEN)
				invitation.sessionName[bi++] = buffer.peek(i++);
			invitation.sessionName[bi++] = '\0';
			if (buffer.peek(i++) != 0x00)
			{
				//Serial.println("Could not find 0x00, not enough data");
				return 0;
			}

			//Serial.print("Consumed ");
			//Serial.print(i);
			//Serial.println(" bytes");

			buffer.pop(i); // consume all the bytes that made up this message

			session->receivedInvitation(invitation, portType);

			return i;
		}
		else if (0 == memcmp(command, amEndSession, sizeof(amEndSession)))
		{
			//Serial.println("received EndSession");

			// minimum amount : 4 bytes for protocol version, 4 bytes for initiator token, 4 bytes for sender SSRC
			minimumLen += (4 + 4 + 4);
			if (buffer.getLength() < minimumLen) 
				return 0;

			// 2 (stored in network byte order (big-endian))
			byte protocolVersion[4];
			protocolVersion[0] = buffer.peek(i++);
			protocolVersion[1] = buffer.peek(i++);
			protocolVersion[2] = buffer.peek(i++);
			protocolVersion[3] = buffer.peek(i++);
			if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
			{
				//Serial.print("Wrong protocolVersion: 0x");
				//Serial.print(protocolVersion[0], HEX);
				//Serial.print(protocolVersion[1], HEX);
				//Serial.print(protocolVersion[2], HEX);
				//Serial.print(protocolVersion[3], HEX);
				//Serial.println(" was expecting 0x00000002");
				return 0;
			}

			AppleMIDI_EndSession endSession;

			// A random number generated by the session's initiator.
			endSession.initiatorToken = ntohl(buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++));
			// The sender's synchronization source identifier.
			endSession.ssrc = ntohl(buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++));

			//Serial.print("Consumed ");
			//Serial.print(i);
			//Serial.println(" bytes");

			buffer.pop(i); // consume all the bytes that made up this message

			session->receivedEndSession(endSession, portType);

			return i;
		}
		else if (0 == memcmp(command, amSyncronization, sizeof(amSyncronization)))
		{
			//Serial.println("received Syncronization");

			// minimum amount : 4 bytes for sender SSRC, 1 byte for count, 3 bytes padding and 3 times 8 bytes
			minimumLen += (4 + 1 + 3 + (3 * 8));
			if (buffer.getLength() < minimumLen) 
				return 0;

			AppleMIDI_Syncronization syncronization;

			// The sender's synchronization source identifier.
			syncronization.ssrc = ntohl(buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++));
			syncronization.count = buffer.peek(i++);
			buffer.peek(i++); buffer.peek(i++); buffer.peek(i++); // padding, unused
			syncronization.timestamps[0] = ntohll(buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++));
			syncronization.timestamps[1] = ntohll(buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++));
			syncronization.timestamps[2] = ntohll(buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++), buffer.peek(i++));

			//Serial.print("Consumed ");
			//Serial.print(i);
			//Serial.println(" bytes");

			buffer.pop(i); // consume all the bytes that made up this message

			session->receivedSyncronization(syncronization, portType);
		}
#endif
#ifdef MASTER
		else if (0 == memcmp(command, amReceiverFeedback, sizeof(amReceiverFeedback)))
		{
			return 99;
		}
		else if (0 == memcmp(command, amBitrateReceiveLimit, sizeof(amBitrateReceiveLimit)))
		{
			return 99;
		}
		else if (0 == memcmp(command, amInvitationAccepted, sizeof(amInvitationAccepted)))
		{
		return 99;
		}
		else if (0 == memcmp(command, amInvitationRejected, sizeof(amInvitationRejected)))
		{
		return 99;
		}
#endif
		return 0;
	}
};

END_APPLEMIDI_NAMESPACE