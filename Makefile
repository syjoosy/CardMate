# Имя приложения
APP_NAME = CardMate
SRC = main.cpp
BIN = card_mate
ICON_PNG = icon.png
ICON_ICNS = $(ICON_PNG:.png=.icns)

# Пути для .app bundle
APP_BUNDLE = $(APP_NAME).app
CONTENTS = $(APP_BUNDLE)/Contents
MACOS = $(CONTENTS)/MacOS
RESOURCES = $(CONTENTS)/Resources

CXX = g++
CXXFLAGS = -std=c++17 `wx-config --cxxflags`
LDFLAGS = `wx-config --libs`

all: $(BIN) $(APP_BUNDLE)

# Компиляция бинарника
$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Создание ICNS из PNG, если нужно
$(ICON_ICNS): $(ICON_PNG)
	@echo "Создаем ICNS из PNG..."
	@mkdir -p Icon.iconset
	@sips -z 16 16     $< --out Icon.iconset/icon_16x16.png
	@sips -z 32 32     $< --out Icon.iconset/icon_16x16@2x.png
	@sips -z 32 32     $< --out Icon.iconset/icon_32x32.png
	@sips -z 64 64     $< --out Icon.iconset/icon_32x32@2x.png
	@sips -z 128 128   $< --out Icon.iconset/icon_128x128.png
	@sips -z 256 256   $< --out Icon.iconset/icon_128x128@2x.png
	@sips -z 256 256   $< --out Icon.iconset/icon_256x256.png
	@sips -z 512 512   $< --out Icon.iconset/icon_256x256@2x.png
	@sips -z 512 512   $< --out Icon.iconset/icon_512x512.png
	@sips -z 1024 1024 $< --out Icon.iconset/icon_512x512@2x.png
	@iconutil -c icns Icon.iconset
	@mv Icon.icns $(ICON_ICNS)
	@rm -rf Icon.iconset

# Создание .app bundle
$(APP_BUNDLE): $(BIN) $(ICON_ICNS)
	@echo "Создаем .app bundle..."
	@mkdir -p $(MACOS) $(RESOURCES)
	@cp $(BIN) $(MACOS)/
	@cp $(ICON_ICNS) $(RESOURCES)/

# Создаем Info.plist
	@echo "Создаем Info.plist..."
	@echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > $(CONTENTS)/Info.plist
	@echo "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" >> $(CONTENTS)/Info.plist
	@echo "<plist version=\"1.0\">" >> $(CONTENTS)/Info.plist
	@echo "<dict>" >> $(CONTENTS)/Info.plist
	@echo "  <key>CFBundleName</key>" >> $(CONTENTS)/Info.plist
	@echo "  <string>$(APP_NAME)</string>" >> $(CONTENTS)/Info.plist
	@echo "  <key>CFBundleDisplayName</key>" >> $(CONTENTS)/Info.plist
	@echo "  <string>$(APP_NAME)</string>" >> $(CONTENTS)/Info.plist
	@echo "  <key>CFBundleIdentifier</key>" >> $(CONTENTS)/Info.plist
	@echo "  <string>com.example.$(APP_NAME)</string>" >> $(CONTENTS)/Info.plist
	@echo "  <key>CFBundleVersion</key>" >> $(CONTENTS)/Info.plist
	@echo "  <string>1.0</string>" >> $(CONTENTS)/Info.plist
	@echo "  <key>CFBundleExecutable</key>" >> $(CONTENTS)/Info.plist
	@echo "  <string>$(BIN)</string>" >> $(CONTENTS)/Info.plist
	@echo "  <key>CFBundleIconFile</key>" >> $(CONTENTS)/Info.plist
	@echo "  <string>$(ICON_ICNS:.icns=)</string>" >> $(CONTENTS)/Info.plist  # <-- без .icns
	@echo "  <key>CFBundlePackageType</key>" >> $(CONTENTS)/Info.plist
	@echo "  <string>APPL</string>" >> $(CONTENTS)/Info.plist
	@echo "</dict>" >> $(CONTENTS)/Info.plist
	@echo "</plist>" >> $(CONTENTS)/Info.plist

# Чтобы подпись не падала, гарантируем, что Resources не пустой
	@touch $(RESOURCES)/.placeholder

	@echo "Готово! .app bundle создан: $(APP_BUNDLE)"


clean:
	rm -f $(BIN)
	rm -rf $(APP_BUNDLE) *.icns
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
