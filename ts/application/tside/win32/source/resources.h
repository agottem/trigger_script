/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_RESOURCES_H_
#define _TSIDE_RESOURCES_H_


#define TSIDE_D_IDE           100
#define TSIDE_D_EDITOR        101
#define TSIDE_D_SEARCH_TEXT   103
#define TSIDE_D_SEARCH_GOTO   104
#define TSIDE_D_RUN           105
#define TSIDE_D_REFERENCE     106
#define TSIDE_D_REF_PLUGINS   107
#define TSIDE_D_REF_TEMPLATES 108
#define TSIDE_D_REF_SYNTAX    109
#define TSIDE_D_SELECT        110

#define TSIDE_B_IDE_TOOLBAR 500

#define TSIDE_I_GUI_MAIN      1
#define TSIDE_I_GUI_RUN       2
#define TSIDE_I_GUI_REFERENCE 3

#define TSIDE_A_GUI 1000

#define TSIDE_M_IDE                            1500
#define TSIDE_M_IDE_FILE_NEW                   1501
#define TSIDE_M_IDE_FILE_OPEN                  1502
#define TSIDE_M_IDE_FILE_SAVE                  1503
#define TSIDE_M_IDE_FILE_SAVE_AS               1504
#define TSIDE_M_IDE_FILE_SAVE_ALL              1505
#define TSIDE_M_IDE_FILE_CLOSE                 1506
#define TSIDE_M_IDE_FILE_CLOSE_ALL             1507
#define TSIDE_M_IDE_FILE_EXIT                  1508
#define TSIDE_M_IDE_SEARCH_FIND                1509
#define TSIDE_M_IDE_SEARCH_FIND_NEXT           1510
#define TSIDE_M_IDE_SEARCH_FIND_PREVIOUS       1511
#define TSIDE_M_IDE_SEARCH_REPLACE             1512
#define TSIDE_M_IDE_SEARCH_REPLACE_NEXT        1513
#define TSIDE_M_IDE_SEARCH_REPLACE_ALL         1514
#define TSIDE_M_IDE_SEARCH_GOTO                1515
#define TSIDE_M_IDE_SEARCH_PERFORM_GOTO        1516
#define TSIDE_M_IDE_EDIT_UNDO                  1517
#define TSIDE_M_IDE_EDIT_REDO                  1518
#define TSIDE_M_IDE_EDIT_CUT                   1519
#define TSIDE_M_IDE_EDIT_COPY                  1520
#define TSIDE_M_IDE_EDIT_PASTE                 1521
#define TSIDE_M_IDE_EDIT_SELECT_ALL            1522
#define TSIDE_M_IDE_REFERENCE_LANGUAGE_SYNTAX  1523
#define TSIDE_M_IDE_REFERENCE_PLUGIN_FUNCTIONS 1524
#define TSIDE_M_IDE_REFERENCE_TEMPLATES        1525
#define TSIDE_M_IDE_RUN_START                  1526
#define TSIDE_M_IDE_RUN_STOP                   1527
#define TSIDE_M_IDE_RUN_DEBUG                  1528
#define TSIDE_M_IDE_RUN_CONTINUE               1529
#define TSIDE_M_IDE_RUN_STEP                   1530
#define TSIDE_M_IDE_RUN_STEP_INTO              1531
#define TSIDE_M_IDE_RUN_VARIABLES              1532
#define TSIDE_M_IDE_HELP_ABOUT                 1533

#define TSIDE_C_IDE_TOOLBAR 2000

#define TSIDE_C_EDITOR_FILE_TABS 3000

#define TSIDE_C_SEARCH_TEXT_FIND_INPUT    4000
#define TSIDE_C_SEARCH_TEXT_REPLACE_INPUT 4001
#define TSIDE_C_SEARCH_TEXT_FIND_NEXT     4002
#define TSIDE_C_SEARCH_TEXT_FIND_PREV     4003
#define TSIDE_C_SEARCH_TEXT_REPLACE_NEXT  4004
#define TSIDE_C_SEARCH_TEXT_REPLACE_ALL   4005

#define TSIDE_C_SEARCH_GOTO_LINE_INPUT   5000
#define TSIDE_C_SEARCH_GOTO_PERFORM_GOTO 5001

#define TSIDE_C_RUN_INVOCATION_INPUT    6000
#define TSIDE_C_RUN_COMPILE_AND_RUN     6001
#define TSIDE_C_RUN_STOP                6002
#define TSIDE_C_RUN_DEBUG               6003
#define TSIDE_C_RUN_NOTIFICATION_LIST   6004
#define TSIDE_C_RUN_CLEAR_NOTIFICATIONS 6005

#define TSIDE_C_REFERENCE_TABS          7000

#define TSIDE_C_REF_PLUGINS_CATEGORY        8000
#define TSIDE_C_REF_PLUGINS_FUNCTION        8001
#define TSIDE_C_REF_PLUGINS_DOCUMENTATION   8002
#define TSIDE_C_REF_PLUGINS_INSERT_FUNCTION 8003

#define TSIDE_C_REF_TEMPLATES_CATEGORY        9000
#define TSIDE_C_REF_TEMPLATES_TEMPLATE        9001
#define TSIDE_C_REF_TEMPLATES_CONTENT         9002
#define TSIDE_C_REF_TEMPLATES_USE_TEMPLATE    9003

#define TSIDE_C_REF_SYNTAX_SYNTAX  10000
#define TSIDE_C_REF_SYNTAX_CONTENT 10002

#define TSIDE_C_SELECT_TEMPLATES    11001
#define TSIDE_C_SELECT_USE_TEMPLATE 11002

#define TSIDE_S_PRODUCT_NAME                      1
#define TSIDE_S_TS_FILE_EXTENSION                 2
#define TSIDE_S_GENERIC_ERROR_CAPTION             3
#define TSIDE_S_NEW_FILE_TAB_NAME                 4
#define TSIDE_S_ADD_FILE_TAB_ERROR                5
#define TSIDE_S_SAVE_MODIFIED_FILE                6
#define TSIDE_S_PROCEED_CAPTION                   7
#define TSIDE_S_SAVE_FILE_FAILED                  8
#define TSIDE_S_OPEN_FILE_ERROR                   9
#define TSIDE_S_FIND_END_OF_DOCUMENT_REACHED      10
#define TSIDE_S_FIND_COMPLETE                     11
#define TSIDE_S_FIND_START_OF_DOCUMENT_REACHED    12
#define TSIDE_S_CONTINUE_UNSAVED                  13
#define TSIDE_S_UNSAVED_FILES_ON_RUN              14
#define TSIDE_S_CANNOT_RUN                        15
#define TSIDE_S_NO_RUNNABLE_SELECTION             16
#define TSIDE_S_INITIALIZE_RUN_ERROR              17
#define TSIDE_S_RUN_NOTIFICATION_TIME             18
#define TSIDE_S_RUN_NOTIFICATION_MESSAGE          19
#define TSIDE_S_NOTIFICATION_NOT_DISPLAYED        20
#define TSIDE_S_COULD_NOT_ANNOTATE                21
#define TSIDE_S_CANNOT_RUN_VALIDATION_ERRORS      22
#define TSIDE_S_VALIDATION_ERRORS                 23
#define TSIDE_S_VALIDATION_WARNINGS               24
#define TSIDE_S_PROMPT_RUN_VALIDATION_WARNINGS    25
#define TSIDE_S_VALIDATING_SOURCE                 26
#define TSIDE_S_VALIDATION_COMPLETE_WITH_WARNINGS 27
#define TSIDE_S_VALIDATION_COMPLETE_WITH_ERRORS   28
#define TSIDE_S_VALIDATION_COMPLETE_EXECUTING     29
#define TSIDE_S_EXECUTION_ERROR_ENCOUNTERED       30
#define TSIDE_S_EXECUTION_ERROR                   31
#define TSIDE_S_EXECUTION_COMPLETE_WITH_ERRORS    32
#define TSIDE_S_EXECUTION_COMPLETE                33
#define TSIDE_S_CANNOT_PRINT_VARIABLE             34
#define TSIDE_S_CANNOT_CLOSE_WHILE_RUNNING        35
#define TSIDE_S_RUN_IN_PROGRESS                   36
#define TSIDE_S_EXECUTION_MANUALLY_STOPPED        37
#define TSIDE_S_CANNOT_MODIFY_WHILE_RUNNING       38
#define TSIDE_S_TOOLTIP_NEW_FILE                  39
#define TSIDE_S_TOOLTIP_OPEN_FILE                 40
#define TSIDE_S_TOOLTIP_SAVE_FILE                 41
#define TSIDE_S_TOOLTIP_CLOSE_FILE                42
#define TSIDE_S_TOOLTIP_FIND                      43
#define TSIDE_S_TOOLTIP_GOTO                      44
#define TSIDE_S_TOOLTIP_CUT                       45
#define TSIDE_S_TOOLTIP_COPY                      46
#define TSIDE_S_TOOLTIP_PASTE                     47
#define TSIDE_S_TOOLTIP_UNDO                      48
#define TSIDE_S_TOOLTIP_REDO                      49
#define TSIDE_S_TOOLTIP_REFERENCE                 50
#define TSIDE_S_TOOLTIP_START                     51
#define TSIDE_S_TOOLTIP_STOP                      52
#define TSIDE_S_TOOLTIP_DEBUG                     53
#define TSIDE_S_TOOLTIP_CONTINUE                  54
#define TSIDE_S_TOOLTIP_STEP                      55
#define TSIDE_S_TOOLTIP_STEP_INTO                 56
#define TSIDE_S_TOOLTIP_VARIABLES                 57
#define TSIDE_S_REFERENCE_TAB_PLUGINS             58
#define TSIDE_S_REFERENCE_TAB_TEMPLATES           59
#define TSIDE_S_UNCATEGORIZED_FUNCTION_CATEGORY   60
#define TSIDE_S_CANNOT_POPULATE_REFERENCE_DATA    61
#define TSIDE_S_REFERENCE_TAB_SYNTAX              62
#define TSIDE_S_BLANK_TEMPLATE                    63


#endif

