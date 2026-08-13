// Windows-specific drag and drop unit tests.
//
// These drive the registered uiDropTarget through the IDropTarget protocol
// with a mocked IDataObject, which lets us verify the full
// enter -> move -> drop -> data path without going through the OLE drag
// machinery. OLE itself is initialized by uiInit(), so the drop target
// registered by uiControlRegisterDragDestination() is real; only the data
// source is synthetic.
//
// This test is only built on Windows and only works when libui is built as
// a static library (the uiDropTarget class is not exported otherwise).

// The Windows headers must come before unit.h: unit.h pulls in cmocka.h,
// which defines an `inline` macro under MSVC that breaks the C++ standard
// library headers that winapi.hpp includes.
#include "../../windows/uipriv_windows.hpp"
#include "../../windows/droptarget.hpp"

#include <shlobj.h>   // DROPFILES

#include "unit.h"

#include <string>
#include <vector>

// IDataObject mock
//
// Offers CF_TEXT/CF_UNICODETEXT and/or CF_HDROP depending on the payload it
// was constructed with. GetData() produces real HGLOBALs (a DROPFILES
// structure for CF_HDROP) that the Windows backend reads back through
// uiDragContextDragData().

class MockDataObject : public IDataObject {
	public:
		MockDataObject(const wchar_t *text, const wchar_t *const *files, int nFiles)
		{
			m_ref = 1;
			if (text != NULL)
				m_text = text;
			for (int i = 0; i < nFiles; i++)
				m_files.push_back(files[i]);
			if (HasText()) {
				m_formats.push_back(CF_TEXT);
				m_formats.push_back(CF_UNICODETEXT);
			}
			if (HasFiles())
				m_formats.push_back(CF_HDROP);
		}
		~MockDataObject() {}

		bool HasText() const { return !m_text.empty(); }
		bool HasFiles() const { return !m_files.empty(); }

		STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
		{
			if (ppv == NULL)
				return E_POINTER;
			if (riid == IID_IUnknown || riid == IID_IDataObject) {
				*ppv = this;
				AddRef();
				return S_OK;
			}
			*ppv = NULL;
			return E_NOINTERFACE;
		}
		STDMETHODIMP_(ULONG) AddRef() override
		{
			return ++m_ref;
		}
		STDMETHODIMP_(ULONG) Release() override
		{
			if (--m_ref == 0) {
				delete this;
				return 0;
			}
			return m_ref;
		}

		STDMETHODIMP GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium) override
		{
			if (pformatetcIn == NULL || pmedium == NULL)
				return E_INVALIDARG;
			pmedium->tymed = TYMED_HGLOBAL;
			pmedium->pUnkForRelease = NULL;

			if (pformatetcIn->cfFormat == CF_HDROP)
				return HasFiles() ? buildHDROP(pmedium) : DV_E_FORMATETC;
			if (pformatetcIn->cfFormat == CF_UNICODETEXT)
				return HasText() ? buildText(pmedium, TRUE) : DV_E_FORMATETC;
			if (pformatetcIn->cfFormat == CF_TEXT)
				return HasText() ? buildText(pmedium, FALSE) : DV_E_FORMATETC;
			return DV_E_FORMATETC;
		}
		STDMETHODIMP GetDataHere(FORMATETC *pformatetcIn, STGMEDIUM *pmedium) override
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP QueryGetData(FORMATETC *pformatetcIn) override
		{
			size_t i;

			if (pformatetcIn == NULL)
				return E_INVALIDARG;
			if ((pformatetcIn->tymed & TYMED_HGLOBAL) == 0)
				return DV_E_TYMED;
			for (i = 0; i < m_formats.size(); i++)
				if (m_formats[i] == pformatetcIn->cfFormat)
					return S_OK;
			return DV_E_FORMATETC;
		}
		STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pformatetcIn, FORMATETC *pformatetcOut) override
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium, BOOL fRelease) override
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc) override
		{
			if (ppenumFormatEtc == NULL)
				return E_INVALIDARG;
			if (dwDirection != DATADIR_GET)
				return E_NOTIMPL;
			*ppenumFormatEtc = new MockEnumFORMATETC(m_formats);
			return S_OK;
		}
		STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) override
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP DUnadvise(DWORD dwConnection) override
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise) override
		{
			return E_NOTIMPL;
		}

	private:
		HRESULT buildHDROP(STGMEDIUM *pmedium)
		{
			size_t i;
			size_t bytes = sizeof(DROPFILES);
			HGLOBAL hGlobal;
			DROPFILES *df;
			wchar_t *dst;

			for (i = 0; i < m_files.size(); i++)
				bytes += (m_files[i].size() + 1) * sizeof(wchar_t);
			bytes += sizeof(wchar_t);

			hGlobal = GlobalAlloc(GMEM_MOVEABLE, bytes);
			if (hGlobal == NULL)
				return E_OUTOFMEMORY;
			df = (DROPFILES *) GlobalLock(hGlobal);
			if (df == NULL) {
				GlobalFree(hGlobal);
				return E_OUTOFMEMORY;
			}
			df->pFiles = sizeof(DROPFILES);
			df->fWide = TRUE;
			dst = (wchar_t *) ((char *) df + sizeof(DROPFILES));
			for (i = 0; i < m_files.size(); i++) {
				wcscpy(dst, m_files[i].c_str());
				dst += m_files[i].size() + 1;
			}
			*dst = L'\0';
			GlobalUnlock(hGlobal);

			pmedium->hGlobal = hGlobal;
			return S_OK;
		}
		HRESULT buildText(STGMEDIUM *pmedium, BOOL unicode)
		{
			HGLOBAL hGlobal;
			size_t bytes;
			void *data;

			if (unicode)
				bytes = (m_text.size() + 1) * sizeof(wchar_t);
			else {
				int len = WideCharToMultiByte(CP_ACP, 0, m_text.c_str(), (int) m_text.size() + 1, NULL, 0, NULL, NULL);
				bytes = (size_t) len;
			}
			hGlobal = GlobalAlloc(GMEM_MOVEABLE, bytes);
			if (hGlobal == NULL)
				return E_OUTOFMEMORY;
			data = GlobalLock(hGlobal);
			if (data == NULL) {
				GlobalFree(hGlobal);
				return E_OUTOFMEMORY;
			}
			if (unicode)
				memcpy(data, m_text.c_str(), bytes);
			else
				WideCharToMultiByte(CP_ACP, 0, m_text.c_str(), (int) m_text.size() + 1,
					(char *) data, (int) bytes, NULL, NULL);
			GlobalUnlock(hGlobal);

			pmedium->hGlobal = hGlobal;
			return S_OK;
		}

		// IEnumFORMATETC mock
		class MockEnumFORMATETC : public IEnumFORMATETC {
			public:
				MockEnumFORMATETC(const std::vector<CLIPFORMAT> &formats)
				{
					size_t i;

					m_ref = 1;
					m_pos = 0;
					for (i = 0; i < formats.size(); i++) {
						FORMATETC fe;

						fe.cfFormat = formats[i];
						fe.ptd = NULL;
						fe.dwAspect = DVASPECT_CONTENT;
						fe.lindex = -1;
						fe.tymed = TYMED_HGLOBAL;
						m_formats.push_back(fe);
					}
				}
				MockEnumFORMATETC(const std::vector<FORMATETC> &formats)
				{
					m_ref = 1;
					m_pos = 0;
					m_formats = formats;
				}
				~MockEnumFORMATETC() {}

				STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
				{
					if (ppv == NULL)
						return E_POINTER;
					if (riid == IID_IUnknown || riid == IID_IEnumFORMATETC) {
						*ppv = this;
						AddRef();
						return S_OK;
					}
					*ppv = NULL;
					return E_NOINTERFACE;
				}
				STDMETHODIMP_(ULONG) AddRef() override
				{
					return ++m_ref;
				}
				STDMETHODIMP_(ULONG) Release() override
				{
					if (--m_ref == 0) {
						delete this;
						return 0;
					}
					return m_ref;
				}

				STDMETHODIMP Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched) override
				{
					ULONG fetched = 0;

					while (fetched < celt && m_pos < (ULONG) m_formats.size()) {
						rgelt[fetched] = m_formats[m_pos];
						m_pos++;
						fetched++;
					}
					if (pceltFetched != NULL)
						*pceltFetched = fetched;
					return fetched == celt ? S_OK : S_FALSE;
				}
				STDMETHODIMP Skip(ULONG celt) override
				{
					if (m_pos + celt > (ULONG) m_formats.size())
						return S_FALSE;
					m_pos += celt;
					return S_OK;
				}
				STDMETHODIMP Reset(void) override
				{
					m_pos = 0;
					return S_OK;
				}
				STDMETHODIMP Clone(IEnumFORMATETC **ppenum) override
				{
					*ppenum = new MockEnumFORMATETC(m_formats);
					((MockEnumFORMATETC *) *ppenum)->m_pos = m_pos;
					return S_OK;
				}

			private:
				std::vector<FORMATETC> m_formats;
				ULONG m_pos;
				ULONG m_ref;
		};

		std::wstring m_text;
		std::vector<std::wstring> m_files;
		std::vector<CLIPFORMAT> m_formats;
		ULONG m_ref;
};

// shared drag callback state
struct winDragState {
	uiDragDestination *dd;
	int enters;
	int moves;
	int exits;
	int drops;
	uiDragOperation enterResult;
	uiDragOperation moveResult;
	int dropResult;
	// captured inside the enter callback
	int gotX;
	int gotY;
	int gotTypes;
	int gotOps;
	// data to fetch inside the drop callback, and where to store it
	uiDragType fetchType;
	int fetchFailed;
	char *gotText;
	int gotFileCount;
	char **gotFiles;
};

static void winDragStateFree(struct winDragState *s)
{
	free(s->gotText);
	if (s->gotFiles != NULL) {
		int i;

		for (i = 0; i < s->gotFileCount; i++)
			free(s->gotFiles[i]);
		free(s->gotFiles);
	}
}

static char *winDragDup(const char *str)
{
	size_t len = strlen(str) + 1;
	char *out = (char *) malloc(len);

	if (out == NULL)
		return NULL;
	memcpy(out, str, len);
	return out;
}

static uiDragOperation winDragOnEnter(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct winDragState *s = (struct winDragState *) data;

	s->enters++;
	uiDragContextPosition(dc, &s->gotX, &s->gotY);
	s->gotTypes = uiDragContextDragTypes(dc);
	s->gotOps = uiDragContextDragOperations(dc);
	return s->enterResult;
}

static uiDragOperation winDragOnMove(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct winDragState *s = (struct winDragState *) data;

	s->moves++;
	return s->moveResult;
}

static void winDragOnExit(uiDragDestination *dd, void *data)
{
	struct winDragState *s = (struct winDragState *) data;

	s->exits++;
}

static int winDragOnDrop(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct winDragState *s = (struct winDragState *) data;
	uiDragData *d;
	int i;

	s->drops++;
	if (s->fetchType != 0) {
		d = uiDragContextDragData(dc, s->fetchType);
		if (d == NULL) {
			s->fetchFailed = 1;
			return 0;
		}
		if (d->type == uiDragTypeText) {
			s->gotText = winDragDup(d->data.text);
			if (s->gotText == NULL)
				s->fetchFailed = 1;
		} else if (d->type == uiDragTypeURIs) {
			s->gotFileCount = d->data.URIs.numURIs;
			s->gotFiles = (char **) calloc((size_t) s->gotFileCount, sizeof(char *));
			if (s->gotFiles == NULL)
				s->fetchFailed = 1;
			for (i = 0; i < s->gotFileCount && s->gotFiles != NULL; i++) {
				s->gotFiles[i] = winDragDup(d->data.URIs.URIs[i]);
				if (s->gotFiles[i] == NULL)
					s->fetchFailed = 1;
			}
		}
		uiFreeDragData(d);
	}
	return s->dropResult;
}

// registers a drag destination on the given control and returns the
// registered uiDropTarget
static uiDropTarget *winDragRegister(uiControl *c, struct winDragState *s)
{
	uiDragDestination *dd;

	dd = uiNewDragDestination();
	uiDragDestinationSetAcceptTypes(dd, uiDragTypeText | uiDragTypeURIs);
	uiDragDestinationOnEnter(dd, winDragOnEnter, s);
	uiDragDestinationOnMove(dd, winDragOnMove, s);
	uiDragDestinationOnExit(dd, winDragOnExit, s);
	uiDragDestinationOnDrop(dd, winDragOnDrop, s);
	uiControlRegisterDragDestination(uiControl(c), dd);

	s->dd = dd;
	assert_non_null(dd->priv);
	return (uiDropTarget *) dd->priv;
}

#define uiLabelPtrFromState(s) uiControlPtrFromState(uiLabel, s)

static void winDragTextDrop(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct winDragState s;
	uiDropTarget *dt;
	HWND hwnd;
	MockDataObject *data;
	DWORD effect;
	POINTL pt;
	RECT rect;
	int expectedX;
	int expectedY;

	*c = uiNewLabel("drag destination");
	hwnd = (HWND) uiControlHandle(uiControl(*c));

	memset(&s, 0, sizeof(s));
	dt = winDragRegister(uiControl(*c), &s);

	data = new MockDataObject(L"hello drag", NULL, 0);
	pt.x = 42;
	pt.y = 37;

	effect = DROPEFFECT_COPY;
	s.enterResult = uiDragOperationCopy;
	s.moveResult = uiDragOperationMove;
	s.dropResult = 1;
	s.fetchType = uiDragTypeText;

	assert_int_equal(dt->DragEnter(data, MK_LBUTTON, pt, &effect), S_OK);
	assert_int_equal(s.enters, 1);
	assert_int_equal(s.exits, 0);
	assert_int_equal(s.gotTypes, uiDragTypeText);
	assert_int_equal(s.gotOps, uiDragOperationCopy);
	// uiDragContextPosition() converts from screen to client coordinates
	GetWindowRect(hwnd, &rect);
	expectedX = 42 - rect.left;
	expectedY = 37 - rect.top;
	assert_int_equal(s.gotX, expectedX);
	assert_int_equal(s.gotY, expectedY);
	assert_int_equal(effect, DROPEFFECT_COPY);

	assert_int_equal(dt->DragOver(MK_LBUTTON, pt, &effect), S_OK);
	assert_int_equal(s.moves, 1);
	assert_int_equal(effect, DROPEFFECT_MOVE);

	assert_int_equal(dt->Drop(data, MK_LBUTTON, pt, &effect), S_OK);
	assert_int_equal(s.drops, 1);
	assert_false(s.fetchFailed);
	assert_string_equal(s.gotText, "hello drag");
	assert_int_equal(effect, DROPEFFECT_MOVE);

	assert_int_equal(dt->DragLeave(), S_OK);
	assert_int_equal(s.exits, 1);

	data->Release();
	winDragStateFree(&s);
}

static void winDragUriDrop(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct winDragState s;
	uiDropTarget *dt;
	MockDataObject *data;
	const wchar_t *files[] = { L"C:\\tmp\\a.txt", L"C:\\tmp\\b file.txt" };
	DWORD effect;
	POINTL pt;

	*c = uiNewLabel("drag destination");
	memset(&s, 0, sizeof(s));
	dt = winDragRegister(uiControl(*c), &s);

	data = new MockDataObject(NULL, files, 2);
	pt.x = 0;
	pt.y = 0;

	effect = DROPEFFECT_COPY;
	s.enterResult = uiDragOperationCopy;
	s.dropResult = 1;
	s.fetchType = uiDragTypeURIs;

	assert_int_equal(dt->DragEnter(data, 0, pt, &effect), S_OK);
	assert_int_equal(s.enters, 1);
	assert_int_equal(s.gotTypes, uiDragTypeURIs);

	assert_int_equal(dt->Drop(data, 0, pt, &effect), S_OK);
	assert_int_equal(s.drops, 1);
	assert_false(s.fetchFailed);
	assert_int_equal(s.gotFileCount, 2);
	assert_string_equal(s.gotFiles[0], "C:\\tmp\\a.txt");
	assert_string_equal(s.gotFiles[1], "C:\\tmp\\b file.txt");

	data->Release();
	winDragStateFree(&s);
}

static void winDragRejectWrongTypes(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct winDragState s;
	uiDropTarget *dt;
	MockDataObject *data;
	DWORD effect;
	POINTL pt;

	*c = uiNewLabel("drag destination");
	memset(&s, 0, sizeof(s));
	dt = winDragRegister(uiControl(*c), &s);

	// the destination accepts both text and URIs, but this data object only
	// carries text (the URIs path must be skipped)
	data = new MockDataObject(L"text only", NULL, 0);
	pt.x = 0;
	pt.y = 0;
	effect = DROPEFFECT_COPY;

	assert_int_equal(dt->DragEnter(data, 0, pt, &effect), S_OK);
	assert_int_equal(s.enters, 1);

	data->Release();

	// a data object carrying nothing the destination accepts must be
	// rejected without calling the enter callback
	data = new MockDataObject(NULL, NULL, 0);
	assert_int_equal(dt->DragEnter(data, 0, pt, &effect), E_INVALIDARG);
	assert_int_equal(s.enters, 1);
	assert_int_equal(effect, DROPEFFECT_NONE);

	data->Release();
	winDragStateFree(&s);
}

static void winDragDropRejected(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct winDragState s;
	uiDropTarget *dt;
	MockDataObject *data;
	DWORD effect;
	POINTL pt;

	*c = uiNewLabel("drag destination");
	memset(&s, 0, sizeof(s));
	dt = winDragRegister(uiControl(*c), &s);

	data = new MockDataObject(L"hello drag", NULL, 0);
	pt.x = 0;
	pt.y = 0;

	effect = DROPEFFECT_COPY;
	s.enterResult = uiDragOperationCopy;
	s.dropResult = 0;
	s.fetchType = uiDragTypeText;

	assert_int_equal(dt->DragEnter(data, 0, pt, &effect), S_OK);
	assert_int_equal(dt->Drop(data, 0, pt, &effect), S_OK);
	assert_int_equal(s.drops, 1);
	assert_false(s.fetchFailed);
	assert_string_equal(s.gotText, "hello drag");
	// a rejected drop must clear the drag operation
	assert_int_equal(effect, DROPEFFECT_NONE);
	assert_int_equal(uiDragDestinationLastDragOperation(s.dd), uiDragOperationNone);

	data->Release();
	winDragStateFree(&s);
}

#define winDragUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int platformDragDropRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		winDragUnitTest(winDragTextDrop),
		winDragUnitTest(winDragUriDrop),
		winDragUnitTest(winDragRejectWrongTypes),
		winDragUnitTest(winDragDropRejected),
	};

	return cmocka_run_group_tests_name("dragdrop-win", tests,
		unitTestsSetup, unitTestsTeardown);
}
