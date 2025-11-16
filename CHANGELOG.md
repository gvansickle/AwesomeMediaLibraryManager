# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Started to [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) (#28).

### Fixed

- Fixed empty "PERFORMER" (Artist) column for some files (#78).
- Fixed accesses to LibraryModel from non-GUI threads (#83).
- Fixed false dependency of async LibraryModel metadata refresh on ScanResultsTreeModel population (#68).
- Fixed loss of the frames (HH:MM:FF) field value when working with cue sheet INDEX entries.
  Frames are now the single point of truth for such values (#106).

