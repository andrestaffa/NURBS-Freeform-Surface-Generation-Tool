#include "Window.h"

#include "Log.h"

#include <iostream>


// ---------------------------
// static function definitions
// ---------------------------

void Window::keyMetaCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	CallbackInterface* callbacks = static_cast<CallbackInterface*>(glfwGetWindowUserPointer(window));
	callbacks->keyCallback(key, scancode, action, mods);
}


void Window::mouseButtonMetaCallback(GLFWwindow* window, int button, int action, int mods) {
	CallbackInterface* callbacks = static_cast<CallbackInterface*>(glfwGetWindowUserPointer(window));
	callbacks->mouseButtonCallback(button, action, mods);
}


void Window::cursorPosMetaCallback(GLFWwindow* window, double xpos, double ypos) {
	CallbackInterface* callbacks = static_cast<CallbackInterface*>(glfwGetWindowUserPointer(window));
	callbacks->cursorPosCallback(xpos, ypos);
}


void Window::scrollMetaCallback(GLFWwindow* window, double xoffset, double yoffset) {
	CallbackInterface* callbacks = static_cast<CallbackInterface*>(glfwGetWindowUserPointer(window));
	callbacks->scrollCallback(xoffset, yoffset);
}


void Window::windowSizeMetaCallback(GLFWwindow* window, int width, int height) {
	CallbackInterface* callbacks = static_cast<CallbackInterface*>(glfwGetWindowUserPointer(window));
	callbacks->windowSizeCallback(width, height);
}

void Window::framebufferSizeMetaCallback(GLFWwindow* window, int width, int height) {
	CallbackInterface* callbacks = static_cast<CallbackInterface*>(glfwGetWindowUserPointer(window));
	callbacks->framebufferSizeCallback(width, height);
}

// ----------------------
// non-static definitions
// ----------------------

Window::Window(
	std::shared_ptr<CallbackInterface> callbacks, int width, int height,
	const char* title, GLFWmonitor* monitor, GLFWwindow* share
)
	: window(nullptr)
	, callbacks(callbacks)
{
	// specify OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // needed for mac?
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	// create window
	window = std::unique_ptr<GLFWwindow, WindowDeleter>(glfwCreateWindow(width, height, title, monitor, share));
	if (window == nullptr) {
		Log::error("WINDOW failed to create GLFW window");
		throw std::runtime_error("Failed to create GLFW window.");
	}
	glfwMakeContextCurrent(window.get());

	// initialize OpenGL extensions for the current context (this window)
	if (!gladLoadGL()) {
		throw std::runtime_error("Failed to initialize GLAD");
	}

	// If no callbacks were passed in, then we create & set default ones.
	if (callbacks == nullptr) {
		this->callbacks = std::make_shared<CallbackInterface>();
	}

	connectCallbacks();

}


Window::Window(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share)
	: Window(nullptr, width, height, title, monitor, share)
{}

// Boilerplate ImGui setup code. Informed by:
// https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp
void Window::setupImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// We call GetIO() and then use a void cast to supress compiler warnings
	// about unused variables, since we don't use io.
	// We just want the IM_ASSERT in GetIO() to be run, I guess.
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Uncomment the below if you want to use, e.g., arrow keys to navigate
	// between ImGui widgets.
	// 
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Sets dark colour style.
	// You can use StyleColorsLight() here instead if you want.
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window.get(), true);

	// Here, we pass in the glsl version we are using, which should line up
	// with our OpenGL version. There's not much documentation on this ImGui
	// function, but from what I can tell from other people's comments,
	// tutorials, and reading through imgui_impl_opengl3.cpp myself, this
	// seems to be the right approach.
	ImGui_ImplOpenGL3_Init("#version 330 core");
}

void Window::openDirectory(std::string& dir) {
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileDialog* pFileDialog;
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileDialog, reinterpret_cast<void**>(&pFileDialog));
		if (SUCCEEDED(hr)) {
			DWORD dwOptions;
			hr = pFileDialog->GetOptions(&dwOptions);
			if (SUCCEEDED(hr)) {
				hr = pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
			}
			hr = pFileDialog->Show(NULL);
			if (SUCCEEDED(hr)) {
				IShellItem* pItem;
				hr = pFileDialog->GetResult(&pItem);
				if (SUCCEEDED(hr)) {
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					if (SUCCEEDED(hr)) {
						int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
						std::string filePath(len, '\0');
						WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &filePath[0], len, NULL, NULL);
						dir = filePath;
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileDialog->Release();
		}
		CoUninitialize();
	}
}

void Window::openFile(std::string& fileLocation, const std::vector<COMDLG_FILTERSPEC>& fileTypes) {
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileOpenDialog* pFileOpen;
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
		if (SUCCEEDED(hr)) {
			if (!fileTypes.empty()) hr = pFileOpen->SetFileTypes(fileTypes.size(), &fileTypes[0]);
			if (SUCCEEDED(hr)) {
				hr = pFileOpen->Show(NULL);
				if (SUCCEEDED(hr)) {
					IShellItem* pItem;
					hr = pFileOpen->GetResult(&pItem);
					if (SUCCEEDED(hr)) {
						PWSTR pszFilePath;
						hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
						if (SUCCEEDED(hr)) {
							int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
							std::string filePath(len, '\0');
							WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &filePath[0], len, NULL, NULL);
							fileLocation = filePath;
							CoTaskMemFree(pszFilePath);
						}
						pItem->Release();
					}
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
}

void Window::connectCallbacks() {
	// set userdata of window to point to the object that carries out the callbacks
	glfwSetWindowUserPointer(window.get(), callbacks.get());

	// bind meta callbacks to actual callbacks
	glfwSetKeyCallback(window.get(), keyMetaCallback);
	glfwSetMouseButtonCallback(window.get(), mouseButtonMetaCallback);
	glfwSetCursorPosCallback(window.get(), cursorPosMetaCallback);
	glfwSetScrollCallback(window.get(), scrollMetaCallback);
	glfwSetWindowSizeCallback(window.get(), windowSizeMetaCallback);
	glfwSetFramebufferSizeCallback(window.get(), framebufferSizeMetaCallback);
}


void Window::setCallbacks(std::shared_ptr<CallbackInterface> callbacks_) {
	callbacks = callbacks_;
	connectCallbacks();
}


glm::ivec2 Window::getPos() const {
	int x, y;
	glfwGetWindowPos(window.get(), &x, &y);
	return glm::ivec2(x, y);
}


glm::ivec2 Window::getSize() const {
	int w, h;
	glfwGetWindowSize(window.get(), &w, &h);
	return glm::ivec2(w, h);
}
