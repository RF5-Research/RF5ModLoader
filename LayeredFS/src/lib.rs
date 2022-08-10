use std::{ffi::{c_void, CString, OsString}, error::Error, path::{Path}, mem, os::windows::prelude::{OsStringExt, OsStrExt}, str::FromStr};
use detour::static_detour;
use walkdir::WalkDir;
// use windows::Win32::{Foundation::{HINSTANCE, MAX_PATH}, System::{SystemServices::DLL_PROCESS_ATTACH, LibraryLoader::{GetModuleFileNameW, GetModuleHandleA, GetProcAddress}}};
use windows_sys::{core::PCWSTR, Win32::{Storage::FileSystem::{FILE_ACCESS_FLAGS, FILE_SHARE_MODE, FILE_CREATION_DISPOSITION, FILE_FLAGS_AND_ATTRIBUTES}, Security::SECURITY_ATTRIBUTES, Foundation::{HANDLE, HINSTANCE}, System::{SystemServices::DLL_PROCESS_ATTACH, LibraryLoader::{GetProcAddress, GetModuleHandleA, LoadLibraryA, LoadLibraryW}}}};


static_detour! {
	static Detour_CreateFileW: extern "system" fn(
		PCWSTR,
		FILE_ACCESS_FLAGS,
		FILE_SHARE_MODE,
		*const SECURITY_ATTRIBUTES,
		FILE_CREATION_DISPOSITION,
		FILE_FLAGS_AND_ATTRIBUTES,
		HANDLE
	) -> HANDLE;
}

// Returns a module symbol's absolute address.
pub fn get_module_symbol_address(module: &str, symbol: &str) -> isize {
    unsafe {
        let c_module = CString::new(module).unwrap();
        let c_symbol = CString::new(symbol).unwrap();
  
        let handle = GetModuleHandleA(c_module.as_ptr() as *mut u8);
        match GetProcAddress(handle, c_symbol.as_ptr() as *mut u8) {
            Some(func) => mem::transmute(func),
            None => panic!("Couldn't find module `{}` and/or symbol `{}` in the module as well.", module.to_string(), symbol.to_string()),
        }
    }
}

/// Called when the DLL is attached to the process.
unsafe fn main() -> Result<(), Box<dyn Error>> {
	Detour_CreateFileW.initialize(
		mem::transmute::<isize, extern "system" fn(
			PCWSTR,
			FILE_ACCESS_FLAGS,
			FILE_SHARE_MODE,
			*const SECURITY_ATTRIBUTES,
			FILE_CREATION_DISPOSITION,
			FILE_FLAGS_AND_ATTRIBUTES,
			HANDLE
		) -> HANDLE>(
			get_module_symbol_address("kernel32", "CreateFileW")
		),
		hook_CreateFileW
	)?
	.enable();

	load_plugins();
	Ok(())
}

unsafe fn u16_ptr_to_string(ptr: *const u16) -> OsString {
    let len = (0..).take_while(|&i| *ptr.offset(i) != 0).count();
    let slice = std::slice::from_raw_parts(ptr, len);

    OsString::from_wide(slice)
}

// fn get_data_path() -> &'static Path
// {
// }
fn load_plugins()
{
	let plugins_dir = Path::new("plugins");
	if plugins_dir.exists() {
		for entry in WalkDir::new(plugins_dir).into_iter().filter_map(|e| e.ok()) {
			let path = entry.path();
			if path.is_file() && path.extension().unwrap() == "dll" {
				let module = path
					.as_os_str()
					.encode_wide()
					.chain(Some(0))
					.collect::<Vec<_>>();
				unsafe {
					LoadLibraryW(module.as_ptr());
					println!("Loaded module {}", path.file_stem().unwrap().to_str().unwrap());			
				}
			}
		}
	}
}

fn hook_CreateFileW(
    lpfilename: PCWSTR, 
    dwdesiredaccess: FILE_ACCESS_FLAGS, 
    dwsharemode: FILE_SHARE_MODE, 
    lpsecurityattributes: *const SECURITY_ATTRIBUTES, 
    dwcreationdisposition: FILE_CREATION_DISPOSITION, 
    dwflagsandattributes: FILE_FLAGS_AND_ATTRIBUTES, 
    htemplatefile: HANDLE
) -> HANDLE {
    unsafe {
		let native_filepath = u16_ptr_to_string(lpfilename);
		let filepath = Path::new(&native_filepath);

		let process_path_buf = std::env::current_exe().unwrap();
		let process_path = process_path_buf.as_path();
		let process_name = process_path.file_stem().unwrap();
		let data_dir_name = format!("{}_Data", process_name.to_str().unwrap());
		let data_dir = std::format!(r"{}\{}", process_path.parent().unwrap().to_str().unwrap(), data_dir_name);

		let data_path = Path::new(&data_dir);
		

		if filepath.starts_with(data_path.to_str().unwrap()) {
			let mod_path_str = filepath.to_str().unwrap().replace(data_dir_name.as_str(), "mods");
			let mod_path = Path::new(&mod_path_str);
			if mod_path.exists()  {
				println!("Patching file: {}", mod_path.to_str().unwrap());
				// let buffer = mod_path.to_str().unwrap().encode_utf16().collect::<Vec<u16>>();
				let os_string = OsString::from_str(mod_path.to_str().unwrap())
				.unwrap()
				.encode_wide()
				.chain(Some(0))
				.collect::<Vec<_>>();
				return Detour_CreateFileW.call(
					os_string.as_ptr(),
					dwdesiredaccess,
					dwsharemode,
					lpsecurityattributes,
					dwcreationdisposition,
					dwflagsandattributes,
					htemplatefile
				);
			}
		}
		Detour_CreateFileW.call(
			lpfilename,
			dwdesiredaccess,
			dwsharemode,
			lpsecurityattributes,
			dwcreationdisposition,
			dwflagsandattributes,
			htemplatefile
		)
	}
}


#[no_mangle]
pub extern "system" fn Initialize()
{}

#[no_mangle]
#[allow(non_snake_case)]
pub unsafe extern "system" fn DllMain(
	_module: HINSTANCE,
	call_reason: u32,
	_: *const c_void,
) -> bool {
	if call_reason == DLL_PROCESS_ATTACH {
    	main().is_ok()
	} else {
		false
	}
}