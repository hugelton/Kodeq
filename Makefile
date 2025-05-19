CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
SRCS = parser.cpp simulator.cpp midi_manager.cpp object_factory.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = reelia_simulator

# OS固有のMIDIライブラリリンク設定
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS = -framework CoreMIDI -framework CoreAudio -framework CoreFoundation
else ifeq ($(UNAME_S),Linux)
    # Linux
    LDFLAGS = -lasound -lpthread
else
    # Windows (MinGW/MSYS)
    LDFLAGS = -lwinmm
endif

# RtMidiライブラリパス（必要に応じて変更）
RTMIDI = ./rtmidi

# ヘッダインクルードパス
CXXFLAGS += -I$(RTMIDI)

all: rtmidi_lib $(TARGET)

# RtMidiソースを含める場合
rtmidi_lib:
	@if [ ! -d "$(RTMIDI)" ]; then \
		echo "RtMidiライブラリが見つかりません。ダウンロードしています..."; \
		mkdir -p $(RTMIDI); \
		curl -L https://github.com/thestk/rtmidi/raw/master/RtMidi.h -o $(RTMIDI)/RtMidi.h; \
		curl -L https://github.com/thestk/rtmidi/raw/master/RtMidi.cpp -o $(RTMIDI)/RtMidi.cpp; \
		echo "RtMidiライブラリをダウンロードしました"; \
	fi

$(TARGET): $(OBJS) $(RTMIDI)/RtMidi.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(RTMIDI)/RtMidi.o: $(RTMIDI)/RtMidi.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(RTMIDI)/RtMidi.o

.PHONY: all clean rtmidi_lib