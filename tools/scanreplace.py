
from string import Template


def replace_action(target, source, env):
    with open(str(source[0])) as source_file:
        contents = Template(source_file.read()).safe_substitute(env)
    with open(str(target[0]), "w") as target_file:
        target_file.write(contents)
    return 0


def replace_string(target, source, env):
    return f"building '{target[0]!s}' from '{source[0]!s}'"


def generate(env, **kw):
    action = env.Action(replace_action, replace_string)
    env["BUILDERS"]["ScanReplace"] = env.Builder(
        action=action, src_suffix=".in", single_source=True
    )


def exists(env):
    return 1
