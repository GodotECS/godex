import os


def generate_dynamic_system_funcs():
    """ Generates the functions needed to convert a scripted system in a
    compile time system. """

    # Compilation performances doesn't change much with 100/500/2000 functions,
    # but 500 functions seems already much after the real needs; in the end
    # this cap is only for scripting systems.
    max_dynamic_systems = 500
    path = "./systems/dynamic_system.gen.h"

    if os.path.exists(path):
        # The file already esists, do not generate it again so the compiler
        # will skip this file if already compiled.
        return

    f = open(path, "w")
    f.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
    f.write("#define DYNAMIC_SYSTEMS_MAX " + str(max_dynamic_systems) + "\n")
    f.write("uint32_t registered_dynamic_system_count = 0;\n")
    f.write("godex::DynamicSystemInfo dynamic_info[DYNAMIC_SYSTEMS_MAX];\n")

    # Write functions.
    for i in range(max_dynamic_systems):
        f.write("void dynamic_system_exec_internal_" + str(i) + "(World *p_world) {\n")
        f.write("	godex::DynamicSystemInfo::executor(p_world, dynamic_info[" + str(i) + "]);\n")
        f.write("}\n")
        f.write("\n")
        f.write("void dynamic_system_get_info_internal_" + str(i) + "(SystemExeInfo& r_info) {\n")
        f.write("	godex::DynamicSystemInfo::get_info(dynamic_info[" + str(i) + "], dynamic_system_exec_internal_"+str(i)+", r_info);\n")
        f.write("}\n")

    # Write the array that olds the function pointers.
    f.write("\n")
    f.write("func_get_system_exe_info func_get_dynamic_systems_exe_info_ptr[DYNAMIC_SYSTEMS_MAX] = {\n")
    for i in range(max_dynamic_systems):
        if i > 0:
            f.write(", ")
        f.write("	dynamic_system_get_info_internal_" + str(i) + "\n")
    f.write("};\n")

    f.write("\n")
    f.write("func_system_execute dynamic_systems_ptr[DYNAMIC_SYSTEMS_MAX] = {\n")
    for i in range(max_dynamic_systems):
        if i > 0:
            f.write(", ")
        f.write("	dynamic_system_exec_internal_" + str(i) + "\n")
    f.write("};\n")

    f.write("\n")
    f.write("uint32_t godex::register_dynamic_system() {\n")
    f.write("	const uint32_t id = registered_dynamic_system_count++;\n")
    f.write("	CRASH_COND_MSG(id >= DYNAMIC_SYSTEMS_MAX, \"You can't register more than \" + itos(DYNAMIC_SYSTEMS_MAX) + \" dynamic systems. Please open an issue so we can increase this limit.\");\n")
    f.write("	return id;\n")
    f.write("}\n")

    f.write("\n")
    f.write("func_get_system_exe_info godex::get_func_dynamic_system_exec_info(uint32_t p_id){\n")
    f.write("	CRASH_COND_MSG(p_id >= DYNAMIC_SYSTEMS_MAX, \"The ID \" + itos(p_id) + \" is out of bounds \" + itos(DYNAMIC_SYSTEMS_MAX) + \". Please open an issue so we can increase this limit.\");\n")
    f.write("	return func_get_dynamic_systems_exe_info_ptr[p_id];\n")
    f.write("}\n")

    f.write("\n")
    f.write("godex::DynamicSystemInfo* godex::get_dynamic_system_info(uint32_t p_id){\n")
    f.write("	CRASH_COND_MSG(p_id >= DYNAMIC_SYSTEMS_MAX, \"The ID \" + itos(p_id) + \" is out of bounds \" + itos(DYNAMIC_SYSTEMS_MAX) + \". Please open an issue so we can increase this limit.\");\n")
    f.write("	return &dynamic_info[p_id];\n")
    f.write("}\n")

    f.close()


def internal_generate_system_exe_funcs(is_temporary, max_parameters, path):
    """ Generates the functions needed to process the `System`s. """

    if os.path.exists(path):
        # The file already esists, do not generate it again so the compiler
        # will skip this file if already compiled.
        return

    f = open(path, "w")
    f.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")

    for i in range(max_parameters + 1):
        if i == 0:
            continue

        f.write("template <")
        for p in range(i):
            f.write("class P" + str(p))
            if p < (i-1):
                f.write(", ")
        f.write(">\n")

        if is_temporary:
            f.write("bool temporary_system_exec_func(World *p_world, bool (*p_system)(")
        else:
            f.write("void system_exec_func(World *p_world, void (*p_system)(")

        for p in range(i):
            f.write("P" + str(p))
            if p < (i-1):
                f.write(", ")
        f.write(")) {\n")

        for p in range(i):
            f.write("	OBTAIN(p" + str(p)+", P"+str(p)+", p_world);\n")

        if is_temporary:
            f.write("	return p_system(\n")
        else:
            f.write("	p_system(\n")
        for p in range(i):
            f.write("		p"+str(p)+".inner")
            if p < (i-1):
                f.write(",\n")
        f.write(");\n")

        f.write("}\n")
        f.write("\n")


def generate_system_exe_funcs():
    """ Generates the functions needed to process the `System`s. """

    max_parameters = 30
    path = "./systems/system_exe_funcs.gen.h"

    internal_generate_system_exe_funcs(False, max_parameters, path)


def generate_temporary_system_exe_funcs():
    max_parameters = 30
    path = "./systems/temporary_system_exe_funcs.gen.h"

    internal_generate_system_exe_funcs(True, max_parameters, path)













