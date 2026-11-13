#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TARGET		:=	PKBankVC
BUILD		:=	build
SOURCES		:=	source source/text
INCLUDES	:=	include
GRAPHICS	:=	gfx
ROMFS		:=	romfs
GFXBUILD	:=	$(ROMFS)/gfx

APP_TITLE		:=	PKBankVC
APP_DESCRIPTION	:=	Pokemon Bank for Virtual Console (GB/GBC)
APP_AUTHOR		:=	SilverDemon
APP_ICON		:=	assets/icon-48px.png

CIA_RSF			:=	assets/build-cia.rsf
CIA_BANNER		:=	assets/banner.bnr

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-ffunction-sections \
			$(ARCH)

INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(dir)) \
			-I$(DEVKITPRO)/libctru/include \
			-I$(BUILD)

CFLAGS	+=	$(INCLUDE) -D__3DS__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	:=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(BUILD)/$(TARGET).map

LIBPATHS	:= -L$(DEVKITPRO)/libctru/lib
LIBS		:= -lcitro2d -lcitro3d -lctru -lm

_3DSXFLAGS	:= --smdh=$(TARGET).smdh --romfs=$(ROMFS)

#---------------------------------------------------------------------------------
# files
#---------------------------------------------------------------------------------
CFILES		:= $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
GFXFILES	:= $(foreach dir,$(GRAPHICS),$(wildcard $(dir)/*.t3s))

OFILES		:= $(patsubst %.c,$(BUILD)/%.o,$(notdir $(CFILES)))
HFILES		:= $(patsubst %.t3s,$(BUILD)/%.h,$(notdir $(GFXFILES)))
T3XFILES	:= $(patsubst %.t3s,$(GFXBUILD)/%.t3x,$(notdir $(GFXFILES)))

VPATH		:= $(SOURCES) $(GRAPHICS)

include $(DEVKITPRO)/devkitARM/base_tools
export LD	:= $(CC)

.PHONY: all clean cia

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh $(T3XFILES)
	@3dsxtool $< $@ $(_3DSXFLAGS)
	@echo built ... $(notdir $@)

$(TARGET).smdh: $(APP_ICON)
	@smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(APP_ICON) $@
	@echo built ... $(notdir $@)

$(TARGET).elf: $(OFILES)
	@echo linking $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

$(OFILES): | $(BUILD) $(HFILES)

$(BUILD)/%.o: %.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(BUILD)/$*.d $(CFLAGS) -c $< -o $@

$(BUILD)/%.h $(GFXBUILD)/%.t3x: %.t3s | $(BUILD) $(GFXBUILD)
	@echo $(notdir $<)
	@tex3ds -i $< -H $(BUILD)/$*.h -d $(BUILD)/$*.d -o $(GFXBUILD)/$*.t3x

$(BUILD) $(GFXBUILD):
	@mkdir -p $@

cia: all
	@makerom -f cia -target t -exefslogo -o $(TARGET).cia -elf $(TARGET).elf -rsf $(CIA_RSF) -banner $(CIA_BANNER) -icon $(TARGET).smdh -DAPP_ROMFS="$(ROMFS)"
	@echo built ... $(TARGET).cia

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(GFXBUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).cia

-include $(BUILD)/*.d
