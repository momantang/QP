#pragma once
#include <QIODevice>
#include <QVector>

namespace Flv
{
	using std::unique_ptr;
	using std::shared_ptr;

	double readDouble(QIODevice& in);
	uint32_t readUInt32(QIODevice& in);
	uint32_t readUInt24(QIODevice& in);
	uint32_t readUInt16(QIODevice& in);
	uint8_t readUInt8(QIODevice& in);


	void writeDouble(QIODevice& out, double val);
	void writeUint32(QIODevice& out, uint32_t val);
	void writeUint24(QIODevice& out, uint32_t val);
	void writeUint16(QIODevice& out, uint16_t val);
	void writeUint8(QIODevice& out, uint8_t val);

}
