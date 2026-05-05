# Releasing libyaml

## Version numbering

libyaml follows `MAJOR.MINOR.PATCH` versioning.
Patch releases contain bug fixes and security fixes only.

## Pre-release checklist

- [ ] All CI checks pass on the release branch
- [ ] Security advisories drafted for any fixes (see below)
- [ ] Downstream maintainers notified (see Downstream coordination)

## Update version numbers

Update the version in these files:

- `announcement.msg`
- `Changes` (add release entry with date)
- `CMakeLists.txt` (`YAML_VERSION_MAJOR`, `YAML_VERSION_MINOR`,
  `YAML_VERSION_PATCH`)
- `configure.ac` (`YAML_MAJOR`, `YAML_MINOR`, `YAML_PATCH`,
  `YAML_RELEASE`, `YAML_CURRENT`, `YAML_REVISION`)

Commit and push to `release/0.x.y`.

## Build release archives

The GitHub workflow `.github/workflows/dist.yaml` builds archives
automatically when you push to a `release/*` branch.
It produces `yaml-0.x.y.tar.gz` and `yaml-0.x.y.zip`.

To build manually:

    make docker-dist

Archives are written to `pkg/docker/output/`.
This requires the `yamlio/libyaml-dev` Docker image (build it with
`make docker-build`).

## Merge and tag

    git checkout master
    git merge release/0.x.y
    git tag -a 0.x.y
    # Paste the Changes entry as the tag message
    git push origin master 0.x.y

## Create a GitHub release

1. Go to Releases and click "Draft a new release"
2. Select the tag you just created
3. Title: `v0.x.y`
4. Paste the changelog entry into the description
5. Upload the `.tar.gz` and `.zip` archives
6. Generate SHA-256 checksums and include them in the release notes
7. Publish

## Security release process

For releases that fix security vulnerabilities:

### Before the release

1. Draft a GitHub Security Advisory (GHSA) for each vulnerability
   via the Security tab
2. Request a CVE ID through GitHub (they are a CNA)
3. Set the patched version in the advisory
4. Notify Tier 1 downstream maintainers listed in `Adopters.md`
   so they can prepare updated packages

### On release day

1. Publish all drafted GHSAs
2. Push the signed tag and publish the GitHub release
3. Post to `oss-security@lists.openwall.org`

### After the release

1. Monitor downstream releases (PyYAML wheels, Ruby, distro packages)
2. Update GHSAs with downstream advisory references as they appear

## Downstream coordination

For security releases, notify downstream maintainers before
publishing:

- **Tier 1** (direct consumers, pre-notify): PyYAML, Ruby Psych
- **Tier 2** (distros): `distros@openwall.org` for embargoed issues
- **Tier 3** (other bindings): public announcement on release day

See `Adopters.md` for the full list.

## Update pyyaml.org

See <https://github.com/yaml/pyyaml.org/blob/master/ReadMe.md>.
