# assume that this script is called with working directory in this directory

output_cmake_toolchain_file := toolchain.cmake
output_cmakelist_file 		:= CMakeLists.txt

current_dir := ${CURDIR}
make_build_dir := ${current_dir}/make_build
base_makefile_dir := ${current_dir}/base_makefile

$(shell mkdir -p ${make_build_dir})
$(shell cp ${base_makefile_dir}/*.mk ${make_build_dir})
$(shell find $(make_build_dir) -type f -exec sed -i "s|SDK_ROOT := .*|SDK_ROOT := $(current_dir)/sdk|g" {} \;)
$(shell find $(make_build_dir) -type f -exec sed -i "s|PROJ_DIR := .*|PROJ_DIR := |g" {} \;)
$(shell find $(make_build_dir) -type f -exec sed -i "s|.*foreach target.*||g" {} \;)
$(shell find $(make_build_dir) -type f -exec sed -i "s|.*\.\./config ||g" {} \;)

default: generate_cube_mx_cmakelist generate_cmake_toolchain

include $(make_build_dir)/*.mk


final_c_flag = $(CFLAGS)
# final_c_flag_filtered_1 = $(filter-out $(C_INCLUDES),$(final_c_flag))
final_c_flag_subbed := $(subst ",\\",$(final_c_flag))
# final_c_flag_filtered_final = $(filter-out -MF"$(@:%.o=%.d)",$(final_c_flag_filtered_1))

pattern = -T%
raw_final_ld_flag := $(LDFLAGS)
# final_ld_flag_filtered := $(raw_final_ld_flag)
final_ld_flag_filtered := $(filter-out $(pattern),$(raw_final_ld_flag))
# final_ld_flag_subbed := $(subst -TNRFF407VGTx_FLASH.ld,-T $(current_dir)/NRFF407VGTx_FLASH.ld,$(final_ld_flag_filtered))

# source: https://stackoverflow.com/questions/39758585/duplicate-compile-flag-in-cmake-cxx-flags
generate_cmake_toolchain:
	@echo "" > $(output_cmake_toolchain_file)

	@echo 'function(removeDuplicateSubstring stringIn stringOut)' >> $(output_cmake_toolchain_file)
	@echo 'separate_arguments(stringIn)' >> $(output_cmake_toolchain_file)
	@echo 'list(REMOVE_DUPLICATES stringIn)' >> $(output_cmake_toolchain_file)
	@echo 'string(REPLACE ";" " " stringIn "$${stringIn}")' >> $(output_cmake_toolchain_file)
	@echo 'set($${stringOut} "$${stringIn}" PARENT_SCOPE)' >> $(output_cmake_toolchain_file)
	@echo 'endfunction()' >> $(output_cmake_toolchain_file)
	@echo "" >> $(output_cmake_toolchain_file)

	@echo "set(CMAKE_SYSTEM_NAME Generic)" >> $(output_cmake_toolchain_file)
	@echo "set(CMAKE_SYSTEM_PROCESSOR arm)" >> $(output_cmake_toolchain_file)

	@echo "set(CMAKE_TRY_COMPILE_TARGET_TYPE \"STATIC_LIBRARY\")" >> $(output_cmake_toolchain_file)
	@echo "set(BUILD_SHARED_LIBS OFF)" >> $(output_cmake_toolchain_file)

	@echo "find_program(CMAKE_C_COMPILER arm-none-eabi-gcc)" >> $(output_cmake_toolchain_file)
	@echo "find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++)" >> $(output_cmake_toolchain_file)

	@echo "set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> $(output_cmake_toolchain_file)
	@echo "set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> $(output_cmake_toolchain_file)
	@echo "set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> $(output_cmake_toolchain_file)
	@echo "set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)" >> $(output_cmake_toolchain_file)

	@echo "set(CMAKE_CXX_FLAGS \"$(final_c_flag_subbed)\")" >> $(output_cmake_toolchain_file)
	@echo 'removeDuplicateSubstring($${CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS)' >> $(output_cmake_toolchain_file)
	@echo "set(CMAKE_C_FLAGS \" -std=gnu11 $(final_c_flag_subbed)\")" >> $(output_cmake_toolchain_file)
	@echo 'removeDuplicateSubstring($${CMAKE_C_FLAGS} CMAKE_C_FLAGS)' >> $(output_cmake_toolchain_file)
	# -x assembler-with-cpp
	@echo "set(CMAKE_ASM_FLAGS \"$(ASMFLAGS)\")" >> $(output_cmake_toolchain_file)
	@echo 'removeDuplicateSubstring($${CMAKE_ASM_FLAGS} CMAKE_ASM_FLAGS)' >> $(output_cmake_toolchain_file)

	@echo "set(CMAKE_EXE_LINKER_FLAGS \"$(final_ld_flag_filtered) $(LIB_FILES)\")" >> $(output_cmake_toolchain_file)
	@echo 'removeDuplicateSubstring($${CMAKE_EXE_LINKER_FLAGS} CMAKE_EXE_LINKER_FLAGS)' >> $(output_cmake_toolchain_file)

generate_cube_mx_cmakelist:
	@echo "" > $(output_cmakelist_file)

	@echo "set(NRF_SRCS $(filter-out %main.c,$(SRC_FILES)) CACHE INTERNAL \"nrf source_list\")" >> $(output_cmakelist_file)
	@echo "list(REMOVE_DUPLICATES NRF_SRCS)" >> $(output_cmakelist_file)
	@echo "" >> $(output_cmakelist_file)
	@echo "set(NRF_INCS Src $(INC_FOLDERS) CACHE INTERNAL \"nrf inc list\")" >> $(output_cmakelist_file)
	@echo "list(REMOVE_DUPLICATES NRF_INCS)" >> $(output_cmakelist_file)
