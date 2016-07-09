all : BunkerBuilder

BunkerBuilder : main.cpp *.h
	g++ -std=c++17 $< -lSDL2_image -lSDL2_net -ltiff -ljpeg -lpng -lz -lSDL2_ttf -lfreetype -lSDL2_mixer -lSDL2_test -lsmpeg2 -lvorbisfile -lvorbis -logg -lstdc++ -lSDL2 -lEGL -lGLESv1_CM -lGLESv2 -landroid -llog -I${IPATH}/SDL2 -Wl,--no-undefined -shared -o $@