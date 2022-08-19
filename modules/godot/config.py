def can_build(env, platform):
    env.module_add_dependencies("godot", ["godex"])
    return True


def configure(env):
    pass
