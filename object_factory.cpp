#include "base_object.hpp"
#include "midi_object.hpp"

// ObjectFactoryの実装
BaseObject* ObjectFactory::createObject(const std::string &type) {
    if (type == "int") {
        return new IntObject(0);
    } else if (type == "seq") {
        return new SeqObject();
    } else if (type == "count") {
        return new CountObject();
    } else if (type == "binary") {
        return new BinaryPatternObject(0);
    }
    else if (type == "midi_note") {
        return new MIDINoteObject();
    } 
    else if (type == "midi_cc") {
        return new MIDICCObject();
    } 
    else if (type == "midi_seq") {
        return new MIDISeqObject();
    }
    // 将来的に他のタイプを追加

    throw std::runtime_error("Unknown object type: " + type);
}