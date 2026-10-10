execute_process(
    COMMAND "${TEST_PROGRAM}" --actor-array-unwind-termination
    RESULT_VARIABLE actual_exit_code
)
if(NOT "${actual_exit_code}" STREQUAL "86")
    message(FATAL_ERROR
        "Enemy array unwind did not terminate after the second exception: ${actual_exit_code}"
    )
endif()
