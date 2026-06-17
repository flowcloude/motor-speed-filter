import fitz
import glob

for fname in glob.glob(r'c:\Users\Administrator\Desktop\motor\*.pdf'):
    print(f"===== {fname} =====")
    doc = fitz.open(fname)
    for i, page in enumerate(doc):
        text = page.get_text()
        if text.strip():
            print(text[:800])
        print(f"--- page {i+1} ---")
    print(f"Pages: {len(doc)}")
    print()
