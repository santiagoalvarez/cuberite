project (Cuberite)

include_directories ("${PROJECT_SOURCE_DIR}/../")

SET (SRCS
	BeaconEntity.cpp
	BlockEntity.cpp
	BlockEntityWithItems.cpp
	BrewingstandEntity.cpp
	ChestEntity.cpp
	CommandBlockEntity.cpp
	DispenserEntity.cpp
	DropSpenserEntity.cpp
	DropperEntity.cpp
	EnderChestEntity.cpp
	FlowerPotEntity.cpp
	FurnaceEntity.cpp
	HopperEntity.cpp
	JukeboxEntity.cpp
	MobHeadEntity.cpp
	MobSpawnerEntity.cpp
	NoteEntity.cpp
	SignEntity.cpp
)

SET (HDRS
	BeaconEntity.h
	BlockEntity.h
	BlockEntityWithItems.h
	BrewingstandEntity.h
	ChestEntity.h
	CommandBlockEntity.h
	DispenserEntity.h
	DropSpenserEntity.h
	DropperEntity.h
	EnderChestEntity.h
	FlowerPotEntity.h
	FurnaceEntity.h
	HopperEntity.h
	JukeboxEntity.h
	MobHeadEntity.h
	MobSpawnerEntity.h
	NoteEntity.h
	SignEntity.h
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set_source_files_properties(BeaconEntity.cpp PROPERTIES COMPILE_FLAGS "-Wno-error=switch-enum")
endif()

if(NOT MSVC)
	add_library(BlockEntities ${SRCS} ${HDRS})
endif()
