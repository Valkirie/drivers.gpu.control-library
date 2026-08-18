//===========================================================================
// Copyright (C) 2022 Intel Corporation
//
//
//
// SPDX-License-Identifier: MIT
//--------------------------------------------------------------------------

/**
 *
 * @file  Display_Feature_Reset_Sample_App.cpp
 * @brief : This file contains the 'main' function and the Display Feature Reset Sample App. Program execution begins and ends there.
 *
 */

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <assert.h>

#define CTL_APIEXPORT // caller of control API DLL shall define this before
                      // including igcl_api.h
#include "igcl_api.h"
#include "GenericIGCLApp.h"

ctl_result_t GResult = CTL_RESULT_SUCCESS;

/***************************************************************
 * @brief
 * Display Reset Feature Test
 * @param hDisplayOutput
 * @return ctl_result_t
 ***************************************************************/
ctl_result_t DisplayResetFeatureTest(ctl_display_output_handle_t hDisplayOutput)
{
    ctl_result_t Result                                 = CTL_RESULT_SUCCESS;
    ctl_display_feature_reset_t DisplayFeatureResetArgs = { 0 };

    DisplayFeatureResetArgs.Size         = sizeof(ctl_display_feature_reset_t);
    DisplayFeatureResetArgs.Version      = 0;
    DisplayFeatureResetArgs.ResetFeature = CTL_DISPLAY_FEATURE_RESET_FLAG_SCALING | CTL_DISPLAY_FEATURE_RESET_FLAG_WIRE_FORMAT | CTL_DISPLAY_FEATURE_RESET_FLAG_COLOR |
                                           CTL_DISPLAY_FEATURE_RESET_FLAG_LACE | CTL_DISPLAY_FEATURE_RESET_FLAG_VRR | CTL_DISPLAY_FEATURE_RESET_FLAG_QUANTIZATION_RANGE |
                                           CTL_DISPLAY_FEATURE_RESET_FLAG_CONTENT_TYPE | CTL_DISPLAY_FEATURE_RESET_FLAG_PSR | CTL_DISPLAY_FEATURE_RESET_FLAG_AUDIO;
    Result = ctlDisplayFeatureReset(hDisplayOutput, &DisplayFeatureResetArgs);
    LOG_AND_EXIT_ON_ERROR(Result, "ctlDisplayFeatureReset");

Exit:
    return Result;
}

/***************************************************************
 * @brief EnumerateDisplayHandles
 * Only for demonstration purpose, API is called for each of the display output handle in below snippet.
 * User has to filter through the available display output handle and has to call the API with particular display output handle.
 * @param hDisplayOutput, DisplayCount
 * @return ctl_result_t
 ***************************************************************/
ctl_result_t EnumerateDisplayHandles(ctl_display_output_handle_t *hDisplayOutput, uint32_t DisplayCount)
{
    ctl_result_t Result = CTL_RESULT_SUCCESS;

    for (uint32_t DisplayIndex = 0; DisplayIndex < DisplayCount; DisplayIndex++)
    {
        ctl_display_properties_t DisplayProperties = {};
        DisplayProperties.Size                     = sizeof(ctl_display_properties_t);

        EXIT_ON_MEM_ALLOC_FAILURE(hDisplayOutput[DisplayIndex], "hDisplayOutput");

        Result = ctlGetDisplayProperties(hDisplayOutput[DisplayIndex], &DisplayProperties);

        LOG_AND_EXIT_ON_ERROR(Result, "ctlGetDisplayProperties");

        bool IsDisplayAttached = DisplayProperties.DisplayConfigFlags & CTL_DISPLAY_CONFIG_FLAG_DISPLAY_ATTACHED;

        if (FALSE == IsDisplayAttached)
        {
            printf("Display %d is not attached, skipping the call for this display\n", DisplayIndex);
            continue;
        }
        else
        {
            printf("Attached Display Count: %d\n", DisplayIndex);
        }

        // Reset display features for the current attached display

        Result = DisplayResetFeatureTest(hDisplayOutput[DisplayIndex]);

        STORE_AND_RESET_ERROR(Result);
    }

Exit:
    return Result;
}

/***************************************************************
 * @brief EnumerateTargetDisplays
 * Enumerates all the possible target display's for the adapters
 * @param hDisplayOutput, AdapterCount, hDevices
 * @return ctl_result_t
 ***************************************************************/
ctl_result_t EnumerateTargetDisplays(ctl_display_output_handle_t *hDisplayOutput, uint32_t AdapterCount, ctl_device_adapter_handle_t *hDevices)
{
    ctl_result_t Result   = CTL_RESULT_SUCCESS;
    uint32_t DisplayCount = 0;

    for (uint32_t AdapterIndex = 0; AdapterIndex < AdapterCount; AdapterIndex++)
    {
        // enumerate all the possible target display's for the adapters
        // first step is to get the count
        DisplayCount = 0;

        Result = ctlEnumerateDisplayOutputs(hDevices[AdapterIndex], &DisplayCount, hDisplayOutput);

        if (CTL_RESULT_SUCCESS != Result)
        {
            printf("ctlEnumerateDisplayOutputs returned failure code: 0x%X\n", Result);
            STORE_AND_RESET_ERROR(Result);
            continue;
        }
        else if (DisplayCount <= 0)
        {
            printf("Invalid Display Count. skipping display enumration for adapter:%d\n", AdapterIndex);
            continue;
        }

        hDisplayOutput = (ctl_display_output_handle_t *)malloc(sizeof(ctl_display_output_handle_t) * DisplayCount);

        EXIT_ON_MEM_ALLOC_FAILURE(hDisplayOutput, "hDisplayOutput");

        Result = ctlEnumerateDisplayOutputs(hDevices[AdapterIndex], &DisplayCount, hDisplayOutput);

        if (CTL_RESULT_SUCCESS != Result)
        {
            printf("ctlEnumerateDisplayOutputs returned failure code: 0x%X\n", Result);
            STORE_AND_RESET_ERROR(Result);
        }

        // Only for demonstration purpose, API is called for each of the display output handle in below snippet.
        // User has to filter through the available display output handle and has to call the API with particular display output handle.
        Result = EnumerateDisplayHandles(hDisplayOutput, DisplayCount);

        if (CTL_RESULT_SUCCESS != Result)
        {
            printf("EnumerateDisplayHandles returned failure code: 0x%X\n", Result);
        }

        CTL_FREE_MEM(hDisplayOutput);
    }

Exit:

    CTL_FREE_MEM(hDisplayOutput);
    return Result;
}

/***************************************************************
 * @brief Main Function which calls the Sample Power feature API
 * @param
 * @return int
 ***************************************************************/
int main()
{
    ctl_result_t Result                                     = CTL_RESULT_SUCCESS;
    ctl_device_adapter_handle_t *hDevices                   = NULL;
    ctl_display_output_handle_t *hDisplayOutput             = NULL;
    ctl_device_adapter_properties_t DeviceAdapterProperties = { 0 };
    // Get a handle to the DLL module.
    uint32_t AdapterCount = 0;
    uint32_t DisplayCount = 0;
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    ctl_init_args_t CtlInitArgs;
    ctl_api_handle_t hAPIHandle;
    CtlInitArgs.AppVersion = CTL_MAKE_VERSION(CTL_IMPL_MAJOR_VERSION, CTL_IMPL_MINOR_VERSION);
    CtlInitArgs.flags      = 0;
    CtlInitArgs.Size       = sizeof(CtlInitArgs);
    CtlInitArgs.Version    = 0;
    ZeroMemory(&CtlInitArgs.ApplicationUID, sizeof(ctl_application_id_t));

    try
    {
        Result = ctlInit(&CtlInitArgs, &hAPIHandle);
        LOG_AND_EXIT_ON_ERROR(Result, "ctlInit");
    }
    catch (const std::bad_array_new_length &e)
    {
        printf("%s \n", e.what());
    }

    // Initialization successful
    // Get the list of Intel Adapters
    try
    {
        Result = ctlEnumerateDevices(hAPIHandle, &AdapterCount, hDevices);
        LOG_AND_EXIT_ON_ERROR(Result, "ctlEnumerateDevices");
    }
    catch (const std::bad_array_new_length &e)
    {
        printf("%s \n", e.what());
    }

    hDevices = (ctl_device_adapter_handle_t *)malloc(sizeof(ctl_device_adapter_handle_t) * AdapterCount);
    EXIT_ON_MEM_ALLOC_FAILURE(hDevices, "hDevices");

    try
    {
        Result = ctlEnumerateDevices(hAPIHandle, &AdapterCount, hDevices);
        LOG_AND_EXIT_ON_ERROR(Result, "ctlEnumerateDevices");
    }
    catch (const std::bad_array_new_length &e)
    {
        printf("%s \n", e.what());
    }

    Result = EnumerateTargetDisplays(hDisplayOutput, AdapterCount, hDevices);

    if (CTL_RESULT_SUCCESS != Result)
    {
        printf("EnumerateTargetDisplays returned failure code: 0x%X\n", Result);
        STORE_AND_RESET_ERROR(Result);
    }

Exit:

    ctlClose(hAPIHandle);
    CTL_FREE_MEM(hDisplayOutput);
    CTL_FREE_MEM(hDevices);
    printf("Overrall test result is 0x%X\n", GResult);
    return GResult;
}
