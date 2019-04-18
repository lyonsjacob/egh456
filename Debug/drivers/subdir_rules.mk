################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
drivers/Kentec320x240x16_ssd2119_spi.obj: /home/jordikitto/ti/tivaware_c_series_2_1_4_178/examples/boards/ek-tm4c1294xl-boostxl-kentec-s1/drivers/Kentec320x240x16_ssd2119_spi.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"/home/jordikitto/ti/ccs901/ccs/tools/compiler/ti-cgt-arm_18.12.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/home/jordikitto/ti/tivaware_c_series_2_1_4_178/examples/boards/ek-tm4c1294xl-boostxl-kentec-s1" --include_path="/home/jordikitto/ti_rtos/empty_min_EK_TM4C1294XL_TI" --include_path="/home/jordikitto/ti_rtos/empty_min_EK_TM4C1294XL_TI" --include_path="/home/jordikitto/ti/tirtos_tivac_2_16_00_08/products/TivaWare_C_Series-2.1.1.71b" --include_path="/home/jordikitto/ti/tirtos_tivac_2_16_00_08/products/bios_6_45_01_29/packages/ti/sysbios/posix" --include_path="/home/jordikitto/ti/ccs901/ccs/tools/compiler/ti-cgt-arm_18.12.1.LTS/include" --define=TARGET_IS_TM4C129_RA0 --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=ccs --define=TIVAWARE -g --gcc --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="drivers/$(basename $(<F)).d_raw" --obj_directory="drivers" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

drivers/touch.obj: /home/jordikitto/ti/tivaware_c_series_2_1_4_178/examples/boards/ek-tm4c1294xl-boostxl-kentec-s1/drivers/touch.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"/home/jordikitto/ti/ccs901/ccs/tools/compiler/ti-cgt-arm_18.12.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/home/jordikitto/ti/tivaware_c_series_2_1_4_178/examples/boards/ek-tm4c1294xl-boostxl-kentec-s1" --include_path="/home/jordikitto/ti_rtos/empty_min_EK_TM4C1294XL_TI" --include_path="/home/jordikitto/ti_rtos/empty_min_EK_TM4C1294XL_TI" --include_path="/home/jordikitto/ti/tirtos_tivac_2_16_00_08/products/TivaWare_C_Series-2.1.1.71b" --include_path="/home/jordikitto/ti/tirtos_tivac_2_16_00_08/products/bios_6_45_01_29/packages/ti/sysbios/posix" --include_path="/home/jordikitto/ti/ccs901/ccs/tools/compiler/ti-cgt-arm_18.12.1.LTS/include" --define=TARGET_IS_TM4C129_RA0 --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=ccs --define=TIVAWARE -g --gcc --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="drivers/$(basename $(<F)).d_raw" --obj_directory="drivers" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


