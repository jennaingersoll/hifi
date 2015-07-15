//
//  PacketHeaders.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 6/28/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketHeaders.h"

#include <math.h>

#include <QtCore/QDebug>

using namespace PacketType;

const QSet<PacketType::Value> NON_VERIFIED_PACKETS = QSet<PacketType::Value>()
    << NodeJsonStats << EntityQuery
    << OctreeDataNack << EntityEditNack
    << DomainListRequest << StopNode;

const QSet<PacketType::Value> SEQUENCE_NUMBERED_PACKETS = QSet<PacketType::Value>() << AvatarData;

const QSet<PacketType::Value> NON_SOURCED_PACKETS = QSet<PacketType::Value>()
    << StunResponse << CreateAssignment << RequestAssignment
    << DomainServerRequireDTLS << DomainConnectRequest
    << DomainList << DomainConnectionDenied
    << DomainServerPathQuery << DomainServerPathResponse
    << DomainServerAddedNode
    << ICEServerPeerInformation << ICEServerQuery << ICEServerHeartbeat
    << ICEPing << ICEPingReply
    << AssignmentClientStatus;

int arithmeticCodingValueFromBuffer(const char* checkValue) {
    if (((uchar) *checkValue) < 255) {
        return *checkValue;
    } else {
        return 255 + arithmeticCodingValueFromBuffer(checkValue + 1);
    }
}

int numBytesArithmeticCodingFromBuffer(const char* checkValue) {
    if (((uchar) *checkValue) < 255) {
        return 1;
    } else {
        return 1 + numBytesArithmeticCodingFromBuffer(checkValue + 1);
    }
}

int packArithmeticallyCodedValue(int value, char* destination) {
    if (value < 255) {
        // less than 255, just pack our value
        destination[0] = (uchar) value;
        return 1;
    } else {
        // pack 255 and then recursively pack on
        ((unsigned char*)destination)[0] = 255;
        return 1 + packArithmeticallyCodedValue(value - 255, destination + 1);
    }
}

PacketVersion versionForPacketType(PacketType::Value packetType) {
    switch (packetType) {
        case MicrophoneAudioNoEcho:
        case MicrophoneAudioWithEcho:
            return 3;
        case SilentAudioFrame:
            return 5;
        case MixedAudio:
            return 2;
        case InjectAudio:
            return 2;
        case AvatarData:
            return 7;
        case AvatarIdentity:
            return 2;
        case EnvironmentData:
            return 3;
        case DomainList:
        case DomainListRequest:
            return 6;
        case DomainConnectRequest:
            return 2;
        case CreateAssignment:
        case RequestAssignment:
            return 3;
        case OctreeStats:
            return 2;
        case OctreeDataNack:
            return 2;
        case StopNode:
            return 2;
        case EntityAdd:
        case EntityEdit:
        case EntityData:
            return VERSION_ENTITIES_NEW_PROTOCOL_LAYER;
        case EntityEditNack:
            return 2;
        case EntityErase:
            return 3;
        case AudioStreamStats:
            return 2;
        case ICEServerHeartbeat:
        case ICEServerQuery:
            return 2;
        default:
            return 1;
    }
}

#define PACKET_TYPE_NAME_LOOKUP(x) case x:  return QString(#x);

QString nameForPacketType(PacketType::Value packetType) {
    switch (packetType) {
            PACKET_TYPE_NAME_LOOKUP(Unknown);
            PACKET_TYPE_NAME_LOOKUP(StunResponse);
            PACKET_TYPE_NAME_LOOKUP(DomainList);
            PACKET_TYPE_NAME_LOOKUP(Ping);
            PACKET_TYPE_NAME_LOOKUP(PingReply);
            PACKET_TYPE_NAME_LOOKUP(KillAvatar);
            PACKET_TYPE_NAME_LOOKUP(AvatarData);
            PACKET_TYPE_NAME_LOOKUP(InjectAudio);
            PACKET_TYPE_NAME_LOOKUP(MixedAudio);
            PACKET_TYPE_NAME_LOOKUP(MicrophoneAudioNoEcho);
            PACKET_TYPE_NAME_LOOKUP(MicrophoneAudioWithEcho);
            PACKET_TYPE_NAME_LOOKUP(BulkAvatarData);
            PACKET_TYPE_NAME_LOOKUP(SilentAudioFrame);
            PACKET_TYPE_NAME_LOOKUP(EnvironmentData);
            PACKET_TYPE_NAME_LOOKUP(DomainListRequest);
            PACKET_TYPE_NAME_LOOKUP(RequestAssignment);
            PACKET_TYPE_NAME_LOOKUP(CreateAssignment);
            PACKET_TYPE_NAME_LOOKUP(DomainConnectionDenied);
            PACKET_TYPE_NAME_LOOKUP(MuteEnvironment);
            PACKET_TYPE_NAME_LOOKUP(AudioStreamStats);
            PACKET_TYPE_NAME_LOOKUP(DataServerConfirm);
            PACKET_TYPE_NAME_LOOKUP(OctreeStats);
            PACKET_TYPE_NAME_LOOKUP(Jurisdiction);
            PACKET_TYPE_NAME_LOOKUP(JurisdictionRequest);
            PACKET_TYPE_NAME_LOOKUP(AvatarIdentity);
            PACKET_TYPE_NAME_LOOKUP(AvatarBillboard);
            PACKET_TYPE_NAME_LOOKUP(DomainConnectRequest);
            PACKET_TYPE_NAME_LOOKUP(DomainServerRequireDTLS);
            PACKET_TYPE_NAME_LOOKUP(NodeJsonStats);
            PACKET_TYPE_NAME_LOOKUP(EntityQuery);
            PACKET_TYPE_NAME_LOOKUP(EntityData);
            PACKET_TYPE_NAME_LOOKUP(EntityErase);
            PACKET_TYPE_NAME_LOOKUP(OctreeDataNack);
            PACKET_TYPE_NAME_LOOKUP(StopNode);
            PACKET_TYPE_NAME_LOOKUP(AudioEnvironment);
            PACKET_TYPE_NAME_LOOKUP(EntityEditNack);
            PACKET_TYPE_NAME_LOOKUP(SignedTransactionPayment);
            PACKET_TYPE_NAME_LOOKUP(ICEServerHeartbeat);
            PACKET_TYPE_NAME_LOOKUP(DomainServerAddedNode);
            PACKET_TYPE_NAME_LOOKUP(ICEServerQuery);
            PACKET_TYPE_NAME_LOOKUP(ICEServerPeerInformation);
            PACKET_TYPE_NAME_LOOKUP(ICEPing);
            PACKET_TYPE_NAME_LOOKUP(ICEPingReply);
            PACKET_TYPE_NAME_LOOKUP(EntityAdd);
            PACKET_TYPE_NAME_LOOKUP(EntityEdit);
        default:
            return QString("Type: ") + QString::number((int)packetType);
    }
    return QString("unexpected");
}

int numBytesForPacketHeader(const QByteArray& packet) {
    PacketType::Value packetType = packetTypeForPacket(packet);
    return numBytesForPacketHeaderGivenPacketType(packetType);
}

int numBytesForPacketHeader(const char* packet) {
    PacketType::Value packetType = packetTypeForPacket(packet);
    return numBytesForPacketHeaderGivenPacketType(packetType);
}

int numBytesForArithmeticCodedPacketType(PacketType::Value packetType) {
    return (int) ceilf((float) packetType / 255);
}

int numBytesForPacketHeaderGivenPacketType(PacketType::Value packetType) {
    return numBytesForArithmeticCodedPacketType(packetType)
    + numHashBytesForType(packetType)
    + numSequenceNumberBytesForType(packetType)
    + NUM_STATIC_HEADER_BYTES;
}

int numHashBytesForType(PacketType::Value packetType) {
    return (NON_VERIFIED_PACKETS.contains(packetType) ? 0 : NUM_BYTES_MD5_HASH);
}

int numSequenceNumberBytesForType(PacketType::Value packetType) {
    return (SEQUENCE_NUMBERED_PACKETS.contains(packetType) ? sizeof(PacketSequenceNumber) : 0);
}

QUuid uuidFromPacketHeader(const QByteArray& packet) {
    return QUuid::fromRfc4122(packet.mid(numBytesArithmeticCodingFromBuffer(packet.data()) + sizeof(PacketVersion),
                                         NUM_BYTES_RFC4122_UUID));
}

int hashOffsetForPacketType(PacketType::Value packetType) {
    return numBytesForArithmeticCodedPacketType(packetType) + NUM_STATIC_HEADER_BYTES;
}

int sequenceNumberOffsetForPacketType(PacketType::Value packetType) {
    return numBytesForPacketHeaderGivenPacketType(packetType) - sizeof(PacketSequenceNumber);
}

PacketSequenceNumber sequenceNumberFromHeader(const QByteArray& packet, PacketType::Value packetType) {
    if (packetType == PacketType::Unknown) {
        packetType = packetTypeForPacket(packet);
    }

    PacketSequenceNumber result = DEFAULT_SEQUENCE_NUMBER;

    if (SEQUENCE_NUMBERED_PACKETS.contains(packetType)) {
        memcpy(&result, packet.data() + sequenceNumberOffsetForPacketType(packetType), sizeof(PacketSequenceNumber));
    }

    return result;
}

PacketType::Value packetTypeForPacket(const QByteArray& packet) {
    return (PacketType::Value) arithmeticCodingValueFromBuffer(packet.data());
}

PacketType::Value packetTypeForPacket(const char* packet) {
    return (PacketType::Value) arithmeticCodingValueFromBuffer(packet);
}
