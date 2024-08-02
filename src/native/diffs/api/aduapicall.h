#pragma once

#define API_CALL_PROLOG() \
	clear_errors();       \
	try                   \
	{
#define API_CALL_EPILOG()                                                            \
	}                                                                                \
	catch (std::exception & e)                                                       \
	{                                                                                \
		std::string error_text = std::string(__FUNCTION__) + " failed. Exception: "; \
		error_text += e.what();                                                      \
		ADU_LOG("Caught std::exception. Msg: {}", error_text.c_str());               \
		add_error(errors::error_code::standard_library_exception, error_text);       \
	}                                                                                \
	catch (errors::user_exception & e)                                               \
	{                                                                                \
		ADU_LOG(                                                                     \
			"Caught errors::user_exception in {}. Code: {}, Msg: {}",                \
			__FUNCTION__,                                                            \
			(int)e.get_error(),                                                      \
			e.get_message());                                                        \
		add_error(e.get_error(), e.get_message());                                   \
	}                                                                                \
	return get_error_count();
