# Adopters

Known consumers of libyaml.
If your project uses libyaml, please open a PR to add it.

## Tier 1 -- Direct consumers with major downstream impact

These projects account for the vast majority of libyaml's real-world
exposure.
They are notified in advance of security releases.

- **[PyYAML](https://github.com/yaml/pyyaml)** -- Python YAML
  library.
  The `_yaml` C extension wraps libyaml; PyPI wheels bundle it
  statically.
  Downstream: Ansible, AWS CLI, Kubernetes client tooling, Home
  Assistant, SaltStack, Jupyter.

- **[Ruby Psych](https://github.com/ruby/psych)** -- Ruby's default
  YAML parser, ships with Ruby.
  Downstream: Rails, Jekyll, Bundler, RubyGems, Chef, Puppet.

## Tier 2 -- Linux distributions

Distribution packages are coordinated via established infrastructure:

- **Debian / Ubuntu** -- `libyaml-0-2`, `libyaml-dev`
- **Fedora / RHEL** -- `libyaml`, `libyaml-devel`
- **Alpine** -- `yaml`, `yaml-dev`
- **Arch** -- `libyaml`
- **SUSE / openSUSE** -- `libyaml-0-2`, `libyaml-devel`

For embargoed security issues, we use
[distros@openwall.org](https://oss-security.openwall.org/wiki/mailing-lists/distros).

## Tier 3 -- Other language bindings

- **[YAML::XS](https://metacpan.org/pod/YAML::XS)** -- Perl
- **[lyaml](https://github.com/gvvaughan/lyaml)** -- Lua
- **[yaml](https://cran.r-project.org/package=yaml)** -- R
- **[yaml](https://www.php.net/manual/en/book.yaml.php)** -- PHP
  extension
- **[yamerl](https://github.com/yakaz/yamerl)** -- Erlang
- **[libyaml-safer](https://github.com/simonask/libyaml-safer)** --
  Rust
- **[HsYAML](https://hackage.haskell.org/package/HsYAML)** -- Haskell

## Tier 4 -- Transitive consumers

These projects use libyaml through one of the above bindings.
They are not coordinated with directly but benefit from upstream
releases:

Ansible, AWS CLI, Kubernetes ecosystem, Rails, Jekyll, Chef, Puppet,
SaltStack, Home Assistant, MkDocs, Jupyter, and many more.
