def can_build(env, platform):
    env.module_add_dependencies("bullet_physics", ["godex"])
    return True


def configure(env):
    pass
