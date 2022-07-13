#pragma once
#include <QIODevice>
#include <QVector>
#include <QFileDevice>
namespace Flv
{
	using std::unique_ptr;
	using std::shared_ptr;

	double readDouble(QIODevice& in);
	uint32_t readUInt32(QIODevice& in);
	uint32_t readUInt24(QIODevice& in);
	uint16_t readUInt16(QIODevice& in);
	uint8_t readUInt8(QIODevice& in);


	void writeDouble(QIODevice& out, double val);
	void writeUint32(QIODevice& out, uint32_t val);
	void writeUint24(QIODevice& out, uint32_t val);
	void writeUint16(QIODevice& out, uint16_t val);
	void writeUint8(QIODevice& out, uint8_t val);

	namespace TagType
	{
		enum
		{
			Audio = 8,
			Video = 9,
			Script = 18
		};
	}

	namespace SoundFormat
	{
		enum { AAC = 10 };
	};

	namespace AacPacketTYpe {
		enum
		{
			SequenceHeader = 0,
			Raw = 1
		};
	}

	namespace VideoFrameType
	{
		enum
		{
			Keyframe = 1,
			InterFrame = 2,
			DisposableInterFrame = 3,
			GeneratedKeyframe = 4,
			VideoInfoOrCmdFrame = 5
		};
	}

	namespace VideoCodecId
	{
		enum
		{
			AVC = 7,
			HEVC = 12
		};
	}

	namespace AVcPacketType
	{
		enum
		{
			SequenceHeader = 0,
			NALU = 1,
			EndOfSequence = 2
		};
	}

	namespace AmfValueType
	{
		enum
		{
			Number = 0,
			Boolean = 1,
			String = 2,
			Object = 3,
			MovieClip = 4,
			Null = 5,
			Undefined = 6,
			Reference = 7,
			EcmaArray = 8,
			ObjectEndMark = 9,
			StrictArray = 10,
			Date = 11,
			LongString = 12,

		};
	}
	class FileHeader
	{
	public:
		static constexpr auto BytesCnt = 9;
		bool valid;
		uint8_t version;

		union
		{
			struct
			{
				uint8_t video : 1;
				uint8_t reservdl : 1;
				uint8_t audio : 1;
				uint8_t reserved5 : 5;
			};
			uint8_t typeFlags;
		};
		uint32_t dataOffset;
		FileHeader(QIODevice& in);
		void writeTo(QIODevice& out);
	};
	class TagHeader
	{
	public:
		static  constexpr int BytesCnt = 11;

		union
		{
			struct
			{
				uint8_t tagType : 5;
				uint8_t filter : 1;
				uint8_t reserved : 2;
			};
			uint8_t flags;
		};
		uint32_t dataSize;
		int timestamp;

		TagHeader() {}
		bool readFrom(QIODevice& in);
		void writeTo(QIODevice& out);

		TagHeader(QIODevice& in)
		{
			readFrom(in);
		}
		TagHeader(int tagType, uint32_t dataSize, int timestamp) :flags(static_cast<uint8_t>(tagType)), dataSize(dataSize), timestamp(timestamp)
		{

		}
	};
	class AudioTagHeader
	{
	public:
		QByteArray rawData;
		bool isAacSequenceHeader;
		AudioTagHeader(QIODevice& in);
		void writeTo(QIODevice& out);
	};
	class VideoTagHeader
	{
	public:
		QByteArray rawData;
		uint8_t codecId;
		uint8_t frameType;
		uint8_t avcPacketType;
		bool isKeyFrame();
		bool isAvcSequenceHeader();

		VideoTagHeader(QIODevice& in);
		void writeTo(QIODevice& out);
	};

	void writeAvcEndOfSeqTag(QIODevice& out, int timestamp);

	class AmfValue
	{
	public:
		const int type;
		AmfValue(int type) :type(type) {}
		~AmfValue() = default;
		virtual  void writeTo(QIODevice& out)
		{
			writeUint8(out, static_cast<uint8_t>(type));
		}
	};

	unique_ptr<AmfValue> readAmfValue(QIODevice& in);

	class ScriptBody
	{
	public:
		unique_ptr<AmfValue> name;
		unique_ptr<AmfValue> value;

		ScriptBody(QIODevice& in);
		bool isOnMetaData() const;

		void writeTo(QIODevice& out);
	};
	class AmfNumber :public AmfValue
	{
	public:
		double val;
		AmfNumber(double val) :AmfValue(AmfValueType::Number), val(val) {}
		AmfNumber(QIODevice& in) :AmfValue(AmfValueType::Number)
		{
			val = readDouble(in);
		}
		void writeTo(QIODevice& out) override
		{
			AmfValue::writeTo(out);
			writeDouble(out, val);
		}
	};
	class Amfboolean :public AmfValue
	{
	public:
		bool val;
		Amfboolean(bool val_) :AmfValue(AmfValueType::Boolean), val(val_) {}
		Amfboolean(QIODevice& in) :AmfValue(AmfValueType::Boolean) { val = readUInt8(in); }
		void writeTo(QIODevice& out) override { AmfValue::writeTo(out); writeUint8(out, val); }
	};
	class AmfString :public AmfValue
	{
	public:
		QByteArray data;
		AmfString(QByteArray data_) :AmfValue(AmfValueType::String), data(std::move(data_)) {}
		AmfString(QIODevice& in);
		void writeTo(QIODevice& out) override;
		static void writeStrWithoutValType(QIODevice& out, const QByteArray& data);
	};
	class AmfLongString :public AmfValue
	{
	public:
		QByteArray data;
		AmfLongString(QIODevice& in);
		void writeTo(QIODevice& out) override;
	};
	class AmfReference :public AmfValue
	{
	public:
		uint16_t val;
		AmfReference(QIODevice& in) :AmfValue(AmfValueType::Reference) { val = readUInt16(in); }
		void writeTo(QIODevice& out) override { AmfValue::writeTo(out); writeUint16(out, val); }
	};
	class AmfDate :public AmfValue
	{
	public:
		QByteArray rawData;
		AmfDate(QIODevice& in) :AmfValue(AmfValueType::Date)
		{
			rawData = in.read(10);
		}
		void writeTo(QIODevice& out) override
		{
			AmfValue::writeTo(out);
			out.write(rawData);
		}
	};

	class AmfObjectProperty
	{
	public:
		QByteArray name;
		unique_ptr<AmfValue> value;
		AmfObjectProperty() {}
		AmfObjectProperty(QIODevice& in);
		void write(QIODevice& out);
		static void write(QIODevice& out, const QByteArray& name, AmfValue* value);

		bool isObjectEnd();
		static void writeObjectEndTo(QIODevice& out);
	};
	class AmfEcmaArray :public AmfValue
	{
		std::vector<AmfObjectProperty> properties;
	public:
		AmfEcmaArray() :AmfValue(AmfValueType::EcmaArray) {}
		AmfEcmaArray(std::vector<AmfObjectProperty>&& properties);

		AmfEcmaArray(QIODevice& in);
		void writeTo(QIODevice& out) override;
		unique_ptr<AmfValue>& operator[](QByteArray name);
	};

	class AmfObject :public AmfValue
	{
		std::vector<AmfObjectProperty> properties;
	public:
		AmfObject() :AmfValue(AmfValueType::Object) {}
		AmfObject(QIODevice& in);
		void writeTo(QIODevice& out) override;

		unique_ptr<AmfEcmaArray> moveToEcmaArray();

		class ReservedArrayAnchor
		{
			QByteArray name;
			int currentSize = 0;
			QIODevice* outDev = nullptr;
			qint64 arrBeginPos;
			qint64 arrEndPos;

			QByteArray spaceNmae()const;
		public:
			const int maxSize;
			ReservedArrayAnchor(QByteArray name, int maxSize) :name(std::move(name)), maxSize(maxSize) {}
			int size() { return currentSize; }
			void writeTo(QIODevice& out);
			void appendNumber(double val);
		};
		shared_ptr<ReservedArrayAnchor> insertReservedNumberArray(QByteArray name, int maxSize);
	private:
		std::vector<shared_ptr<ReservedArrayAnchor>> anchors;
	};

	class AmfStrictArray :public  AmfValue
	{
	public:
		std::vector<unique_ptr<AmfValue>> values;
		AmfStrictArray() :AmfValue(AmfValueType::StrictArray) {}
		AmfStrictArray(QIODevice& in);
		void writeTo(QIODevice& out) override;
	};
	class AnchoredAmfNumber:public AmfValue
	{
	public:
		class Anchor
		{
			friend class AnchoredAmfNumber;
			qint64 pos;
			QIODevice *outDev=nullptr;
		public:
			Anchor(){}
			void update(double val);
		};
		AnchoredAmfNumber(double val=0);
		void writeTo(QIODevice& out) override;
		shared_ptr<Anchor> getAnchor();
	private:
		shared_ptr<Anchor> anchor;
	};
}

class FlvLiveDownloadDelegate
{
	QIODevice& in;
	std::unique_ptr<QFileDevice> out;
public:
	static constexpr  auto MaxKeyframes = 6000;
	static constexpr  auto LeastKeyframeInterval = 2500;
	using CreateFileHandler = std::function<std::unique_ptr<QFileDevice>>;

	FlvLiveDownloadDelegate(QIODevice& in, CreateFileHandler create_file_handler);
	~FlvLiveDownloadDelegate();

	bool newDataArrived();
	void stop();

	QString errorString();
	qint64 getDurationInMSec();
	qint64 getReadBytesCnt();
private:
	enum class Error { NoError, FlvParseError, SaveFileOpenError, HevcNotSupported };
	Error error = Error::NoError;

	enum class State { Begin, ReadingTagHeader, ReadingTagBody, ReadingDummy, Stopped };
	State state = State::Begin;
	qint64 bytesRequired;
	qint64 readBytesCnt = 0;
	CreateFileHandler createFileHandler;

	bool openNewFileToWrite();
	void closeFile();

	void updateMetaDataKeyframes(qint64 filePos, int timeInMSec);
	void updateMetaDataDuration();

	bool handleFileHeader();
	bool handleTagHeader();
	bool handleTagBody();
	bool handleScriptTagbody();
	bool handleAudioTagBody();
	bool handleVideoTagBody();

	bool isTimestampBaseValid = false;
	int timestampBase;
	int curFileAudioDuration = 0;
	int curFileVideoduration = 0;
	qint64 totalDuration = 0;
	int prevKeyframeTimestamp = -LeastKeyframeInterval;

	Flv::TagHeader tagHeader;

	using FlvArrayAnchor=Flv::AmfObject::ReservedArrayAnchor;
	using FlvNumberAnchor=Flv::AnchoredAmfNumber::Anchor;

	std::unique_ptr<Flv::ScriptBody> onMetaDataScript;
	QByteArray fileHeaderBuffer;
	QByteArray aacSeqHeaderBuffer;
	QByteArray avcSeqHeaderBuffer;


};
