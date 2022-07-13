//#include "FLV.h"
//#include <QtEndian>
//#include <QCoreApplication>
//#include <QFileDevice>
//#include <QBuffer>
//#include <QIODevice>
//
//using std::shared_ptr;
//using std::make_shared;
//using std::unique_ptr;
//using std::make_unique;
//
//double Flv::readDouble(QIODevice& in)
//{
//	char data[8];
//	in.read(data, 8);
//	return qFromBigEndian<double>(data);
//}
//
//uint32_t Flv::readUInt32(QIODevice& in)
//{
//	char data[4];
//	in.read(data, 4);
//	return qFromBigEndian<uint32_t>(data);
//}
//
//uint32_t Flv::readUInt24(QIODevice& in)
//{
//	char data[4];
//	data[0] = 0;
//	in.read(data + 1, 3);
//	return qFromBigEndian<uint32_t>(data);
//}
//
//uint16_t Flv::readUInt16(QIODevice& in)
//{
//	char data[2];
//	in.read(data, 2);
//	return qFromBigEndian<uint16_t>(data);
//}
//
//uint8_t Flv::readUInt8(QIODevice& in)
//{
//	uint8_t data;
//	in.read(reinterpret_cast<char*>(&data), 1);
//	return data;
//}
//
//void Flv::writeDouble(QIODevice& out, double val)
//{
//	val = qToBigEndian<double>(val);
//	out.write(reinterpret_cast<char*>(&val), 8);
//}
//
//void Flv::writeUint32(QIODevice& out, uint32_t val)
//{
//	val = qToBigEndian<uint32_t>(val);
//	out.write(reinterpret_cast<char*>(&val), 8);
//}
//
//void Flv::writeUint24(QIODevice& out, uint32_t val)
//{
//	val = qToBigEndian<uint32_t>(val);
//	out.write(reinterpret_cast<char*>(&val) + 1, 3);
//}
//
//void Flv::writeUint16(QIODevice& out, uint16_t val)
//{
//	val = qToBigEndian<uint16_t>(val);
//	out.write(reinterpret_cast<char*> (&val), 2);
//}
//
//void Flv::writeUint8(QIODevice& out, uint8_t val)
//{
//	out.write(reinterpret_cast<char*>(&val), 1);
//}
//
//void Flv::writeAvcEndOfSeqTag(QIODevice& out, int timestamp)
//{
//	TagHeader(TagType::Video, 5, timestamp).writeTo(out);
//	const char TagData[5] = {
//		0x17,
//		0x02,
//		0x00,
//		0x00,
//		0x00
//	};
//	out.write(TagData, 5);
//	writeUint32(out, 16);
//}
//
//unique_ptr<Flv::AmfValue> Flv::readAmfValue(QIODevice& in)
//{
//	auto type = readUInt8(in);
//	switch (type)
//	{
//	case AmfValueType::Number:return make_unique<AmfNumber>(in);
//	case AmfValueType::Boolean:return make_unique<Amfboolean>(in);
//	case AmfValueType::String:return make_unique<AmfString>(in);
//	case AmfValueType::Object:return make_unique<AmfObject>(in);
//	case AmfValueType::MovieClip:return make_unique<AmfValue>(AmfValueType::MovieClip);
//	case AmfValueType::Null:return make_unique<AmfValue>(AmfValueType::Null);
//	case AmfValueType::Undefined:return make_unique<AmfValue>(AmfValueType::Undefined);
//	case AmfValueType::Reference:return make_unique<AmfReference>(in);
//	case AmfValueType::EcmaArray:return make_unique<AmfEcmaArray>(in);
//	case AmfValueType::ObjectEndMark:return make_unique<AmfValue>(AmfValueType::ObjectEndMark);
//	case AmfValueType::StrictArray:return make_unique<AmfStrictArray>(in);
//	case AmfValueType::Date:return make_unique<AmfDate>(in);
//	case AmfValueType::LongString:return make_unique<AmfLongString>(in);
//	default:return make_unique<AmfValue>(AmfValueType::Null);
//	}
//}
//
//Flv::ScriptBody::ScriptBody(QIODevice& in)
//{
//	name = readAmfValue(in);
//	value = readAmfValue(in);
//}
//
//bool Flv::ScriptBody::isOnMetaData() const
//{
//	if (name->type != AmfValueType::String)
//	{
//		return false;
//	}
//	return static_cast<AmfString*>(name.get())->data == "onMetaData";
//}
//
//void Flv::ScriptBody::writeTo(QIODevice& out)
//{
//	name->writeTo(out);
//	value->writeTo(out);
//}
//
//Flv::FileHeader::FileHeader(QIODevice& in)
//{
//	constexpr uint32_t FlvSignatureUInt32 = 'F' | ('L' << 8) | ('V' << 16);
//	char signature[4] = { 0 };
//	in.read(signature, 3);
//	if (*reinterpret_cast<uint32_t*>(signature) != FlvSignatureUInt32)
//	{
//		valid = false;
//		return;
//	}
//	valid = true;
//	version = readUInt8(in);
//	typeFlags = readUInt8(in);
//	dataOffset = readUInt32(in);
//}
//
//void Flv::FileHeader::writeTo(QIODevice& out)
//{
//	out.write("FLV", 3);
//	writeUint8(out, version);
//	writeUint8(out, typeFlags);
//	writeUint32(out, dataOffset);
//}
//
//bool Flv::TagHeader::readFrom(QIODevice& in)
//{
//	flags = readUInt8(in);
//	if (filter)
//	{
//		return false;
//	}
//	dataSize = readUInt24(in);
//	auto tsLow = readUInt24(in);
//	auto tsExt = readUInt8(in);
//	timestamp = (tsExt << 24) | tsLow;
//	readUInt24(in);
//	return true;
//}
//
//void Flv::TagHeader::writeTo(QIODevice& out)
//{
//	writeUint8(out, flags);
//	writeUint24(out, dataSize);
//	writeUint24(out, timestamp & 0x00FFFFFF);
//	writeUint8(out, static_cast<uint8_t>(timestamp >> 24));
//	writeUint24(out, 0);
//}
//
//Flv::AudioTagHeader::AudioTagHeader(QIODevice& in)
//{
//	rawData = in.peek(2);
//	auto flags = readUInt8(in);
//	if (SoundFormat::AAC == ((flags >> 4) & 0xF))
//	{
//		isAacSequenceHeader = (readUInt8(in) == AacPacketTYpe::SequenceHeader);
//	}
//	else
//	{
//		isAacSequenceHeader = false;
//		rawData.chop(1);
//	}
//}
//
//void Flv::AudioTagHeader::writeTo(QIODevice& out)
//{
//	out.write(rawData);
//}
//
//bool Flv::VideoTagHeader::isKeyFrame()
//{
//	return frameType == VideoFrameType::Keyframe;
//}
//
//bool Flv::VideoTagHeader::isAvcSequenceHeader()
//{
//	return (codecId == VideoCodecId::AVC && avcPacketType == AVcPacketType::SequenceHeader);
//}
//
//Flv::VideoTagHeader::VideoTagHeader(QIODevice& in)
//{
//	rawData = in.peek(5);
//	auto byte = readUInt8(in);
//	codecId = byte & 0xF;
//	frameType = (byte >> 4) & 0xF;
//	if (codecId == VideoCodecId::AVC)
//	{
//		avcPacketType = readUInt8(in);
//		readUInt24(in);//compositionTime(SI24)
//	}
//	else
//	{
//		avcPacketType = 0xFF;
//		rawData.chop(4);
//	}
//}
//
//void Flv::VideoTagHeader::writeTo(QIODevice& out)
//{
//	out.write(rawData);
//}
//
//FlvLiveDownloadDelegate::FlvLiveDownloadDelegate(QIODevice& in, CreateFileHandler create_file_handler):in(in),createFileHandler(create_file_handler)
//{
//	bytesRequired=Flv::FileHeader::BytesCnt+4;//FlvFileHeader+prevTagSize(UI32)
//}
//
//FlvLiveDownloadDelegate::~FlvLiveDownloadDelegate()
//{
//	if (out!=nullptr)
//	{
//		closeFile();
//	}
//}
//
//bool FlvLiveDownloadDelegate::newDataArrived()
//{
//	bool noError=true;
//	while (noError)
//	{
//		if (in.bytesAvailable()<bytesRequired)
//		{
//			break;
//		}
//		auto tmp=bytesRequired;
//		switch (state)
//		{
//		case State::Begin:
//			noError=handleFileHeader();
//			break;
//		case State::ReadingTagHeader:
//			noError=handleTagHeader();
//			if (noError)
//			{
//				state=State::ReadingTagBody;
//			}
//
//		}
//	}
//}
//
//void FlvLiveDownloadDelegate::stop()
//{
//}
//
//QString FlvLiveDownloadDelegate::errorString()
//{
//}
//
//qint64 FlvLiveDownloadDelegate::getDurationInMSec()
//{
//}
//
//qint64 FlvLiveDownloadDelegate::getReadBytesCnt()
//{
//}
//
//bool FlvLiveDownloadDelegate::openNewFileToWrite()
//{
//}
//
//void FlvLiveDownloadDelegate::closeFile()
//{
//}
//
//void FlvLiveDownloadDelegate::updateMetaDataKeyframes(qint64 filePos, int timeInMSec)
//{
//}
//
//void FlvLiveDownloadDelegate::updateMetaDataDuration()
//{
//}
//
//bool FlvLiveDownloadDelegate::handleFileHeader()
//{
//}
//
//bool FlvLiveDownloadDelegate::handleTagHeader()
//{
//}
//
//bool FlvLiveDownloadDelegate::handleTagBody()
//{
//}
//
//bool FlvLiveDownloadDelegate::handleScriptTagbody()
//{
//}
//
//bool FlvLiveDownloadDelegate::handleAudioTagBody()
//{
//}
//
//bool FlvLiveDownloadDelegate::handleVideoTagBody()
//{
//}
