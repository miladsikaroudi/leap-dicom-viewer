"""Write a small synthetic CT series (a bone shell around soft tissue) as DICOM.

    python tests/make_phantom.py OUT_DIR

Used by CI to run the viewer end to end without patient data. Needs numpy and pydicom.
"""

import sys
from pathlib import Path

import numpy as np
from pydicom.dataset import FileDataset, FileMetaDataset
from pydicom.uid import CTImageStorage, ImplicitVRLittleEndian, generate_uid

SIZE, SLICES, SPACING_MM = 64, 48, 2.0


def phantom() -> np.ndarray:
    """Hounsfield units: air -1000, soft tissue 40, a 6 mm bone shell at 700."""
    z, y, x = np.mgrid[:SLICES, :SIZE, :SIZE]
    r = np.sqrt((x - SIZE / 2) ** 2 + (y - SIZE / 2) ** 2 + ((z - SLICES / 2) * 1.2) ** 2) * SPACING_MM
    hu = np.full(r.shape, -1000, dtype=np.int16)
    hu[r < 50] = 40
    hu[(r >= 44) & (r < 50)] = 700
    return hu


def main(out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    study, series, frame = generate_uid(), generate_uid(), generate_uid()
    for k, plane in enumerate(phantom()):
        meta = FileMetaDataset()
        meta.MediaStorageSOPClassUID = CTImageStorage
        meta.MediaStorageSOPInstanceUID = generate_uid()
        meta.TransferSyntaxUID = ImplicitVRLittleEndian
        ds = FileDataset(str(out / f"slice{k:03d}.dcm"), {}, file_meta=meta, preamble=b"\0" * 128)
        ds.SOPClassUID, ds.SOPInstanceUID = CTImageStorage, meta.MediaStorageSOPInstanceUID
        ds.StudyInstanceUID, ds.SeriesInstanceUID, ds.FrameOfReferenceUID = study, series, frame
        ds.Modality, ds.PatientName, ds.PatientID = "CT", "Phantom^Synthetic", "PHANTOM"
        ds.InstanceNumber = k + 1
        ds.ImagePositionPatient = [0.0, 0.0, k * SPACING_MM]
        ds.ImageOrientationPatient = [1, 0, 0, 0, 1, 0]
        ds.PixelSpacing = [SPACING_MM, SPACING_MM]
        ds.SliceThickness = SPACING_MM
        ds.Rows = ds.Columns = SIZE
        ds.SamplesPerPixel, ds.PhotometricInterpretation = 1, "MONOCHROME2"
        ds.BitsAllocated, ds.BitsStored, ds.HighBit, ds.PixelRepresentation = 16, 16, 15, 1
        ds.RescaleIntercept, ds.RescaleSlope = 0, 1
        ds.PixelData = plane.tobytes()
        ds.save_as(ds.filename, enforce_file_format=True)
    print(f"wrote {SLICES} slices to {out}")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
