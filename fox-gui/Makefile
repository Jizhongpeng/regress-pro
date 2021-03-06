
TOP_DIR = ..
SOURCE_DIR = ../src

include $(SOURCE_DIR)/makeconfig
include $(SOURCE_DIR)/makesystem

COMPILE = $(CC) $(CFLAGS) $(DEFS) $(INCLUDES)

FOX_INCLUDES = $(shell pkg-config --cflags fox)
FOX_LIBS = $(shell pkg-config --libs fox)

GSL_INCLUDES = $(shell pkg-config --cflags gsl)
GSL_LIBS = $(shell pkg-config --libs gsl)

AGG_INCLUDES = $(shell pkg-config --cflags libagg)
AGG_LIBS = $(shell pkg-config --libs libagg)

LIBS += $(FOX_LIBS) $(GSL_LIBS) $(AGG_LIBS) -lsupc++ -pthread

REG_LIBS = -lmhash -lgmp

ifeq ($(HOST_SYS),Windows)
  LIBS += -lgdi32 -lwsock32 -mwindows
endif

INCLUDES += $(FOX_INCLUDES) $(GSL_INCLUDES) -I$(SOURCE_DIR)

CXXCOMPILE = $(CXX) $(CXXFLAGS) $(DEFS) $(INCLUDES)

SRC_FILES = ProgressInfo.cpp spectrum_plot.cpp disp_fit_window.cpp regress_pro_window.cpp \
	dispers_win.cpp sampling_win.cpp \
	fx_numeric_field.cpp fit_panel.cpp fit_window.cpp colors.cpp \
	plot_units.cpp plot_canvas.cpp utils.cpp dispers_save_dialog.cpp filmstack_window.cpp \
	dispers_ui_edit.cpp icons_all.cpp dispers_chooser.cpp \
	fit_params_utils.cpp multi_sample_recipe.cpp recipe_window.cpp \
	fit_recipe.cpp dispers_edit_window.cpp dispers_ui_utils.cpp \
	filelist_table.cpp dataset_table.cpp \
	dataset_window.cpp batch_window.cpp regress_pro.cpp main.cpp
PRG = regress$(EXE)

REG_SRC_FILES := registration.c registered_app.cpp
REG_OBJ_FILEX := $(REG_SRC_FILES:%.cpp=%.o)
REG_OBJ_FILES := $(REG_OBJ_FILEX:%.c=%.o)

OBJ_FILES := $(SRC_FILES:%.cpp=%.o)
DEP_FILES := $(SRC_FILES:%.cpp=.deps/%.P)

LIBEFIT = $(SOURCE_DIR)/libefit.a

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

ifeq ($(PRODUCT_REGISTRATION),yes)
  OBJ_FILES += $(REG_OBJ_FILES)
  LIBS += $(REG_LIBS)
endif

ifneq ($(ENABLE_RELEASE),yes)
  OBJ_FILES += gitversion.o
  DEFS += -DGIT_BUILD
endif

.PHONY: clean all

all: $(PRG)

include $(SOURCE_DIR)/makerules

$(PRG): $(OBJ_FILES) $(LIBEFIT)
	$(CXX) -o $@ $(OBJ_FILES) $(LIBEFIT) $(LIBS)

gitversion.cpp: $(TOP_DIR)/.git/HEAD $(TOP_DIR)/.git/index
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@

clean:
	rm -f gitversion.cpp $(OBJ_FILES) $(PRG)

-include $(DEP_FILES)
