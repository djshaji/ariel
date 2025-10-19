Create a lv2 host based on lilv and gtk4

User interface with drag and drop support for loading plugins and audio files. The host should support basic transport controls (play, stop, record) and display plugin parameters in a user-friendly manner. Additionally, implement a simple mixer with volume and pan controls for each loaded plugin. Ensure the application is cross-platform and can run on Linux, Windows, and macOS.# GitHub Copilot Instructions

# GitHub Copilot Instructions

# GitHub Copilot Instructions
This repository contains a project to create an LV2 host based on lilv and gtk4. The host features a user interface with drag and drop support for loading plugins and audio files. It includes basic transport controls (play, stop, record) and displays plugin parameters in a user-friendly manner. Additionally, it implements a simple mixer with volume and pan controls for each loaded plugin. The application is designed to be cross-platform, running on Linux, Windows, and macOS.

To get started with this project, follow these steps:

User Interface:
- Use GTK4 to create a user-friendly interface.
- Implement drag and drop functionality for loading LV2 plugins and audio files.
- Design transport controls (play, stop, record) for audio playback.
- Display plugin parameters in a clear and organized manner.
- Create a simple mixer interface with volume and pan controls for each loaded plugin.
- Header bar with title, audio engine on / off toggle, and basic menu options.
- Below that, Gtk pane with plugin list on the left and list of active plugins on the right.

Audio Engine:
- Utilize lilv for LV2 plugin hosting and management.
- Implement audio processing to handle playback, recording, and plugin effects.
- Ensure low-latency audio performance for real-time processing.
- Use Jack or another suitable audio backend for cross-platform audio handling.

Cross-Platform Support:
- Ensure compatibility with Linux, Windows, and macOS.
- Use cross-platform libraries and tools to facilitate development.
- Test the application on all supported platforms to ensure consistent behavior.
Dependencies:
- lilv: For LV2 plugin hosting.
- GTK4: For the graphical user interface.
- Other necessary libraries for audio processing and cross-platform support.
Development:
- Set up a development environment with the required dependencies.
- Follow best practices for coding and documentation.
- Use version control (e.g., Git) to manage changes and collaborate with others.
Testing:
- Implement unit tests for core functionality.
- Perform integration tests to ensure all components work together seamlessly.
- Conduct user testing to gather feedback and improve the user experience.
Contributing:
- Encourage contributions from the community.
- Provide guidelines for contributing, including coding standards and submission processes.
License:
- Choose an appropriate open-source license for the project.
- Include a LICENSE file in the repository.