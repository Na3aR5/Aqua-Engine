#include <aqua/utility/Serializer.h>

aqua::Serializer::~Serializer() {
	if (m_serializeStream.is_open()) {
		m_serializeStream.close();
	}
}

aqua::Status aqua::Serializer::SetTargetFile(const char* filePath, bool clearContents) noexcept {
	if (m_serializeStream.is_open()) {
		m_serializeStream.close();
	}
	m_serializeStream.open(filePath, std::ios::binary | (clearContents ? std::ios::trunc : 0));
	if (!m_serializeStream.is_open()) {
		return Error::FAILED_TO_OPEN_FILE;
	}
	return Success{};
}