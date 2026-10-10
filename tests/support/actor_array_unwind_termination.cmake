execute_process(
    COMMAND "${TEST_PROGRAM}" "${TEST_ARGUMENT}"
    RESULT_VARIABLE actual_exit_code
)
if(NOT "${actual_exit_code}" STREQUAL "86")
    message(FATAL_ERROR
        "Actor array unwind did not terminate after the second exception: ${actual_exit_code}"
    )
endif()
