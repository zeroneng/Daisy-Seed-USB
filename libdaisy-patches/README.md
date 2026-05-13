# libdaisy-patches

This folder stores local patch files for changes made against `libDaisy` without committing the full third-party `libDaisy` repository into this project.

## Why this folder exists

`libDaisy` is a third-party dependency and may be updated in the future.
We do not want to vendor or push the entire `libDaisy` repo here just to preserve a few local modifications.
Instead, we keep patch files that can be reviewed and reapplied when needed.

## Patch files currently included

- `0001-add-usbd-readme.patch`
- `0002-add-usbh-readme.patch`
- `0003-add-hid-readme.patch`

These patches add README files to selected USB-related folders inside `libDaisy`.

## How to apply the patches

### 1. Make sure you are in the `libDaisy` repo

Example:

```bash
cd ~/Developer/libDaisy
```

### 2. Check that your repo is clean

```bash
git status
```

If you already have unrelated changes, either commit/stash them first or apply the patches carefully.

### 3. Apply a patch with `git apply`

From inside the `libDaisy` repo, run:

```bash
git apply /path/to/daisy-usb/libdaisy-patches/0001-add-usbd-readme.patch
```

Repeat for the others:

```bash
git apply /path/to/daisy-usb/libdaisy-patches/0002-add-usbh-readme.patch
git apply /path/to/daisy-usb/libdaisy-patches/0003-add-hid-readme.patch
```

On this system, the full path is typically:

```bash
git apply /mnt/clu-nas/developer/daisy-usb/libdaisy-patches/0001-add-usbd-readme.patch
git apply /mnt/clu-nas/developer/daisy-usb/libdaisy-patches/0002-add-usbh-readme.patch
git apply /mnt/clu-nas/developer/daisy-usb/libdaisy-patches/0003-add-hid-readme.patch
```

### 4. Verify the results

```bash
git status
```

You should see the new README files appear as local changes in `libDaisy`.

## If a patch fails

If `git apply` fails, common reasons are:

- the target file/path already exists
- the upstream repo changed enough that the patch no longer matches cleanly
- you are not in the correct `libDaisy` checkout

### Useful checks

```bash
pwd
git status
```

### Try a dry run first

```bash
git apply --check /mnt/clu-nas/developer/daisy-usb/libdaisy-patches/0001-add-usbd-readme.patch
```

### Force manual review if needed

Open the patch file and create the referenced README manually if the patch no longer applies cleanly.

## Recommendation

Treat these patch files as the source of truth for local `libDaisy` documentation changes associated with this project.
