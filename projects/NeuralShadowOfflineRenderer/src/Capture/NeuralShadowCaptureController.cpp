#include <pch.h>

#include <Capture/NeuralShadowCaptureController.h>
#include <Renderer/OfflineRenderer.h>
#include <Renderer/PSOManager.h>
#include <D3DFramework/GraphicUtilites/CommandList.h>
#include <D3DFramework/GraphicUtilites/BufferReadbackBatch.h>
#include <ShadowData/GroundTruthMetadata.h>
#include <ShadowData/NeuralShadowData.h>

#include <Component/Camera/CameraComponent.h>
#include <Component/RenderObject/MeshComponent.h>
#include <Entity/Camera/TBaseCamera.h>
#include <World/World.h>


std::vector<std::byte> NeuralShadowCaptureController::CopyBytes(const void* data, std::size_t byteCount) {
	std::vector<std::byte> bytes(byteCount);
	if (!bytes.empty()) std::memcpy(bytes.data(), data, byteCount);

	return bytes;
}

bool NeuralShadowCaptureController::BuildNeuralOutputBlocks(
	const FDW_SD::Capture::NSCaptureSamplePlan& sample,
	const FDW_SD::Work::NSWorkDispatchHeader& sourceHeader,
	std::span<const std::byte> sourceWorkBytes,
	std::span<const std::byte> sourcePixelBytes,
	std::span<const std::byte> sourceLightBytes,
	std::span<const std::byte> sourceHardBytes,
	std::span<const std::byte> sourceTargetBytes,
	std::span<const std::byte> sourceTargetValidityBytes,
	std::span<const std::byte> sourceBlockerBytes,
	std::span<const std::byte> sourceInputValidityBytes,
	std::vector<FDW_SD::Capture::NSCaptureOwnedBlock>& blocks,
	std::string& error) {







	if (sourceHeader.ScreenWidth != sample.Metadata.OutputWidth ||
		sourceHeader.ScreenHeight != sample.Metadata.OutputHeight ||
		sourceWorkBytes.size() != static_cast<std::size_t>(sourceHeader.WorkItemCount) * sizeof(FDW_SD::Work::NSWorkItem) ||
		sourceBlockerBytes.size() != static_cast<std::size_t>(sourceHeader.WorkItemCount) * 256u * 2u ||
		sourceInputValidityBytes.size() != static_cast<std::size_t>(sourceHeader.WorkItemCount) * 256u) {
		error = "source blocks do not match the live work header";
		return false;
	}

	std::vector<FDW_SD::Work::NSWorkItem> sourceItems(sourceHeader.WorkItemCount);
	std::memcpy(sourceItems.data(), sourceWorkBytes.data(), sourceWorkBytes.size());
	
	const auto regionCountX = FDW_SD::Spatial::RegionCountX(sourceHeader.ScreenWidth);
	const auto regionCountY = FDW_SD::Spatial::RegionCountY(sourceHeader.ScreenHeight);
	
	std::map<std::pair<std::uint32_t, std::uint32_t>, std::vector<std::uint32_t>> grouped;
	for (std::uint32_t index = 0u; index < sourceItems.size(); ++index) {
		const auto& item = sourceItems[index];
	
		const auto patchX = item.ScreenTileIndex % sourceHeader.PatchCountX;
		const auto patchY = item.ScreenTileIndex / sourceHeader.PatchCountX;
		
		const auto regionX = patchX / 4u;
		const auto regionY = patchY / 4u;
		
		if (regionX >= regionCountX || regionY >= regionCountY) {
			error = "source work maps outside the 64x64 region grid";
			return false;
		}
		
		grouped[{ FDW_SD::Spatial::ScreenRegionIndex(regionX, regionY, regionCountX), item.LightIndex }].push_back(index);
	}

	if (grouped.empty()) {
		error = "capture contains no frame/light regions";
		return false;
	}

	std::vector<FDW_SD::Spatial::WorkItem> workItems;
	std::vector<std::uint16_t> lowHard;
	std::vector<std::uint16_t> lowBlocker;
	std::vector<std::uint8_t> lowValidity;
	std::vector<std::uint16_t> receiverFeatures;
	std::vector<std::uint16_t> lightFeatures;
	std::vector<std::uint16_t> targets;
	std::vector<std::uint8_t> targetValidity;
	
	workItems.reserve(grouped.size());
	lowHard.reserve(grouped.size() * FDW_SD::Spatial::LOW_VALUE_COUNT);
	lowBlocker.reserve(grouped.size() * FDW_SD::Spatial::LOW_VALUE_COUNT);
	lowValidity.reserve(grouped.size() * FDW_SD::Spatial::LOW_VALUE_COUNT);
	receiverFeatures.reserve(grouped.size() * FDW_SD::Spatial::RECEIVER_CHANNEL_COUNT * FDW_SD::Spatial::OUTPUT_VALUE_COUNT);
	lightFeatures.reserve(grouped.size() * FDW_SD::Spatial::LIGHT_CHANNEL_COUNT);
	targets.reserve(grouped.size() * FDW_SD::Spatial::OUTPUT_VALUE_COUNT);
	targetValidity.reserve(grouped.size() * FDW_SD::Spatial::OUTPUT_VALUE_COUNT);
	
	const auto oneHalf = FDW_SD::Output::VisibilityEncodingTools::EncodeR16Float(1.0f);

	for (const auto& [identity, sourceIndices] : grouped) {
		const auto workIndex = static_cast<std::uint32_t>(workItems.size());
		const auto& firstSource = sourceItems[sourceIndices.front()];
	
		FDW_SD::Spatial::WorkItem item{};
		item.ScreenRegionIndex = identity.first;
		item.LightIndex = identity.second;
		item.ShadowMetaIndex = firstSource.ShadowMetaIndex;
		item.LowInputOffset = FDW_SD::Spatial::LowInputOffset(workIndex);
		item.OutputOffset = FDW_SD::Spatial::OutputOffset(workIndex);
		workItems.push_back(item);

		std::vector<std::uint16_t> highHard(FDW_SD::Spatial::OUTPUT_VALUE_COUNT, oneHalf);
		std::vector<std::uint16_t> highBlocker(FDW_SD::Spatial::OUTPUT_VALUE_COUNT, oneHalf);
		std::vector<std::uint8_t> highInputValidity(FDW_SD::Spatial::OUTPUT_VALUE_COUNT, 0u);
		std::vector<std::uint16_t> highReceiver(FDW_SD::Spatial::RECEIVER_CHANNEL_COUNT * FDW_SD::Spatial::OUTPUT_VALUE_COUNT, 0u);
		std::vector<std::uint16_t> highTarget(FDW_SD::Spatial::OUTPUT_VALUE_COUNT, oneHalf);
		std::vector<std::uint8_t> highTargetValidity(FDW_SD::Spatial::OUTPUT_VALUE_COUNT, 1u);
		std::array<std::uint16_t, FDW_SD::Spatial::LIGHT_CHANNEL_COUNT> itemLight{};
		
		bool copiedLight = false;
		
		const auto regionX = identity.first % regionCountX;
		const auto regionY = identity.first / regionCountX;
		
		for (const auto sourceIndex : sourceIndices) {
			const auto& sourceItem = sourceItems[sourceIndex];
			if (sourceItem.ShadowMetaIndex != item.ShadowMetaIndex) {
				error = "frame/light region contains inconsistent shadow metadata identity";
				return false;
			}
		
			const auto patchX = sourceItem.ScreenTileIndex % sourceHeader.PatchCountX;
			const auto patchY = sourceItem.ScreenTileIndex / sourceHeader.PatchCountX;
			for (std::uint32_t local = 0u; local < 256u; ++local) {
				const auto globalX = patchX * 16u + local % 16u;
				const auto globalY = patchY * 16u + local / 16u;
			
				if (globalX >= sourceHeader.ScreenWidth || globalY >= sourceHeader.ScreenHeight) continue;
				
				const auto localX = globalX - regionX * FDW_SD::Spatial::OUTPUT_WIDTH;
				const auto localY = globalY - regionY * FDW_SD::Spatial::OUTPUT_HEIGHT;
				
				if (localX >= FDW_SD::Spatial::OUTPUT_WIDTH || localY >= FDW_SD::Spatial::OUTPUT_HEIGHT) {
					error = "source pixel maps outside its selected 64x64 region";
					return false;
				}
				
				const auto destination = FDW_SD::Spatial::OutputIndex(localX, localY);
				const auto sourceLinear = static_cast<std::size_t>(sourceIndex) * 256u + local;
				
				std::memcpy(&highHard[destination], sourceHardBytes.data() + sourceLinear * 2u, 2u);
				std::memcpy(&highBlocker[destination], sourceBlockerBytes.data() + sourceLinear * 2u, 2u);
				
				highInputValidity[destination] = static_cast<std::uint8_t>(sourceInputValidityBytes[sourceLinear]);
				
				for (std::uint32_t channel = 0u; channel < FDW_SD::Spatial::RECEIVER_CHANNEL_COUNT; ++channel) {
					const auto sourceFeature = (static_cast<std::size_t>(sourceIndex) * 10u + channel + 1u) * 256u + local;
					std::memcpy(&highReceiver[channel * FDW_SD::Spatial::OUTPUT_VALUE_COUNT + destination], sourcePixelBytes.data() + sourceFeature * 2u, 2u);
				}
				
				const auto targetSource = static_cast<std::size_t>(sourceItem.OutputOffset) + local;
				
				std::memcpy(&highTarget[destination], sourceTargetBytes.data() + targetSource * 2u, 2u);
				
				highTargetValidity[destination] = static_cast<std::uint8_t>(sourceTargetValidityBytes[targetSource]);
			}

			if (!copiedLight) {
				std::memcpy(itemLight.data(), sourceLightBytes.data() + static_cast<std::size_t>(sourceIndex) * FDW_SD::Spatial::LIGHT_CHANNEL_COUNT * 2u, FDW_SD::Spatial::LIGHT_CHANNEL_COUNT * 2u);
				copiedLight = true;
			}
		}

		if (identity.second >= sample.Lights.size()) {
			error = "work item references a light outside capture metadata";
			return false;
		}
		
		const auto& physicalLight = sample.Lights[identity.second];
		const auto softness = std::clamp(physicalLight.ShadowSoftness, 0.0f, 4.0f);
		const auto encodeNormalized = [](float value) {
			return FDW_SD::Output::VisibilityEncodingTools::EncodeR16Float(std::clamp(value, 0.0f, 1.0f));
		};

		itemLight[5] = encodeNormalized( (physicalLight.LightType == 0u || physicalLight.LightType == 1u) ? physicalLight.SourceRadius * softness / FDW_SD::Spatial::LOCAL_EFFECTIVE_RADIUS_NORMALIZATION : 0.0f);
		itemLight[6] = encodeNormalized(physicalLight.LightType == 3u ? physicalLight.RectangleSize.X * softness / 4.0f : 0.0f);
		itemLight[7] = encodeNormalized(physicalLight.LightType == 3u ? physicalLight.RectangleSize.Y * softness / 4.0f : 0.0f);
		itemLight[12] = encodeNormalized(physicalLight.LightType == 2u ? physicalLight.DirectionalAngularRadius * softness / FDW_SD::Spatial::DIRECTIONAL_EFFECTIVE_RADIUS_NORMALIZATION : 0.0f);

		for (std::uint32_t lowY = 0u; lowY < FDW_SD::Spatial::LOW_HEIGHT; ++lowY) {
			for (std::uint32_t lowX = 0u; lowX < FDW_SD::Spatial::LOW_WIDTH; ++lowX) {
				
				float hardSum = 0.0f;
				std::uint32_t hardCount = 0u;
				float blockerMinimum = 1.0f;
				std::uint32_t blockerCount = 0u;
				
				for (std::uint32_t dy = 0u; dy < FDW_SD::Spatial::UPSCALE_FACTOR; ++dy) {
					for (std::uint32_t dx = 0u; dx < FDW_SD::Spatial::UPSCALE_FACTOR; ++dx) {
				
						const auto high = FDW_SD::Spatial::OutputIndex(lowX * FDW_SD::Spatial::UPSCALE_FACTOR + dx, lowY * FDW_SD::Spatial::UPSCALE_FACTOR + dy);
						const auto flags = highInputValidity[high];
						
						if ((flags & FDW_SD::Spatial::INPUT_VALID_HARD_BIT) != 0u) {
							hardSum += FDW_SD::Output::VisibilityEncodingTools::DecodeR16Float(highHard[high]);
							++hardCount;
						}
						
						if ((flags & FDW_SD::Spatial::INPUT_VALID_BLOCKER_BIT) != 0u) {
							blockerMinimum = std::min(blockerMinimum,
								FDW_SD::Output::VisibilityEncodingTools::DecodeR16Float(highBlocker[high]));
							++blockerCount;
						}
					}
				}
				
				const auto hard = hardCount != 0u ? hardSum / static_cast<float>(hardCount) : 1.0f;
				
				lowHard.push_back(FDW_SD::Output::VisibilityEncodingTools::EncodeR16Float(hard));
				
				lowBlocker.push_back(FDW_SD::Output::VisibilityEncodingTools::EncodeR16Float(blockerCount != 0u ? blockerMinimum : 1.0f));
				
				lowValidity.push_back(static_cast<std::uint8_t>( (hardCount != 0u ? FDW_SD::Spatial::INPUT_VALID_HARD_BIT : 0u) | (blockerCount != 0u ? FDW_SD::Spatial::INPUT_VALID_BLOCKER_BIT : 0u)));
			}
		}

		receiverFeatures.insert(receiverFeatures.end(), highReceiver.begin(), highReceiver.end());
		
		lightFeatures.insert(lightFeatures.end(), itemLight.begin(), itemLight.end());
		targets.insert(targets.end(), highTarget.begin(), highTarget.end());
		targetValidity.insert(targetValidity.end(), highTargetValidity.begin(), highTargetValidity.end());
	}

	FDW_SD::Spatial::Header header{};
	header.GenerationLow = static_cast<std::uint32_t>(sample.GenerationId);
	header.GenerationHigh = static_cast<std::uint32_t>(sample.GenerationId >> 32u);
	header.ScreenWidth = sourceHeader.ScreenWidth;
	header.ScreenHeight = sourceHeader.ScreenHeight;
	header.RegionCountX = regionCountX;
	header.RegionCountY = regionCountY;
	header.WorkItemCount = static_cast<std::uint32_t>(workItems.size());
	header.Completion = 1u;
	
	if (!FDW_SD::Spatial::IsComplete(header, sample.GenerationId)) {
		error = "constructed neural-output header failed completion validation";
		return false;
	}
	
	const auto headerBytes = CopyBytes(&header, sizeof(header));
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputHeader, FDW_SD::Spatial::LAYOUT_VERSION, 1u, headerBytes });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputWorkItems, FDW_SD::Spatial::LAYOUT_VERSION, workItems.size(), CopyBytes(workItems.data(), workItems.size() * sizeof(FDW_SD::Spatial::WorkItem)) });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputLowHard, FDW_SD::Spatial::LAYOUT_VERSION, lowHard.size(), CopyBytes(lowHard.data(), lowHard.size() * sizeof(std::uint16_t)) });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputLowBlocker, FDW_SD::Spatial::LAYOUT_VERSION, lowBlocker.size(), CopyBytes(lowBlocker.data(), lowBlocker.size() * sizeof(std::uint16_t)) });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputLowValidity, FDW_SD::Spatial::LAYOUT_VERSION, lowValidity.size(), CopyBytes(lowValidity.data(), lowValidity.size() * sizeof(std::uint8_t)) });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputReceiverFeatures, FDW_SD::Spatial::LAYOUT_VERSION, receiverFeatures.size(), CopyBytes(receiverFeatures.data(), receiverFeatures.size() * sizeof(std::uint16_t)) });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputLightFeatures, FDW_SD::Spatial::LAYOUT_VERSION, lightFeatures.size(), CopyBytes(lightFeatures.data(), lightFeatures.size() * sizeof(std::uint16_t)) });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputTargetVisibility, FDW_SD::Spatial::LAYOUT_VERSION, targets.size(), CopyBytes(targets.data(), targets.size() * sizeof(std::uint16_t)) });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::NeuralOutputTargetValidity, FDW_SD::Spatial::LAYOUT_VERSION, targetValidity.size(), CopyBytes(targetValidity.data(), targetValidity.size() * sizeof(std::uint8_t)) });
	
	return true;
}

NeuralShadowCaptureController::NeuralShadowCaptureController(NeuralShadowCaptureRunOptions options) : m_xCaptureOptions(std::move(options)) { }

int NeuralShadowCaptureController::GetExitCode() const {
	return m_iCaptureExitCode;
}

bool NeuralShadowCaptureController::IsConfigured() const {
	return m_xCaptureOptions.has_value();
}

bool NeuralShadowCaptureController::IsFinished() const {
	return m_bCaptureFinished;
}

bool NeuralShadowCaptureController::IsWarmupFrame() const {
	return IsConfigured() && !m_bCaptureFinished && m_uCaptureWarmupFramesRemaining != 0u;
}

const NeuralShadowCaptureRunOptions& NeuralShadowCaptureController::GetOptions() const {
	return *m_xCaptureOptions;
}

NSNeuralShadowDataset::NSDatasetResult NeuralShadowDatasetWriter::Publish(
	const NeuralShadowCaptureRunOptions& options,
	const FDW_SD::Capture::NSCaptureSamplePlan& sample,
	std::span<const FDW_SD::Capture::NSCaptureOwnedBlock> blocks) const {

	std::vector<NSNeuralShadowDataset::NSSampleBlockInput> inputs;
	inputs.reserve(blocks.size());
	for (const auto& block : blocks) {
		inputs.push_back({ block.Kind, block.Version, block.ElementCount, block.Bytes });
	}
	
	return NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample(options.DatasetRoot,options.Config.DatasetId,sample.RelativePath,sample.SampleId,sample.GenerationId, inputs);
}

std::uint64_t NeuralShadowCaptureController::GetGenerationOverride() const {
	if (!m_xCaptureOptions || m_bCaptureFinished || m_uCaptureSampleIndex >= m_vCapturePlan.size()) return 0u;
	
	return m_vCapturePlan[m_uCaptureSampleIndex].GenerationId;
}

bool NeuralShadowCaptureController::Initialize(OfflineRenderer& renderer) {
	if (!m_xCaptureOptions) return true;
	
	auto& options = *m_xCaptureOptions;
	auto result = FDW_SD::Capture::CaptureConfiguration::Validate(options.Config);
	if (!result.Succeeded()) {
		m_sCaptureLastError = result.Message;
		return false;
	}
	
	if (!renderer.IsRTSupported() || !renderer.GetDXRDevice()) {
		m_sCaptureLastError = "the selected adapter does not support DirectX Raytracing";
		return false;
	}
	
	if (options.DatasetRoot.empty()) {
		m_sCaptureLastError = "dataset root is required";
		return false;
	}
	
	if (renderer.GetSceneRenderWidth() != static_cast<int>(options.Config.OutputWidth) ||
		renderer.GetSceneRenderHeight() != static_cast<int>(options.Config.OutputHeight)) {
		m_sCaptureLastError = "hidden renderer resolution does not match capture config";
	
		return false;
	}
	
	result = FDW_SD::Capture::CaptureConfiguration::BuildPlan(options.Config, m_vCapturePlan);
	if (!result.Succeeded()) {
		m_sCaptureLastError = result.Message;
		return false;
	}
	
	std::error_code ec;
	std::filesystem::create_directories(options.DatasetRoot, ec);
	if (ec) {
		m_sCaptureLastError = "failed to create dataset root: " + ec.message();
		return false;
	}
	
	const auto patchCount = static_cast<std::uint64_t>(
		FDW_SD::Work::ComputePatchCountX(options.Config.OutputWidth)) *
		FDW_SD::Work::ComputePatchCountY(options.Config.OutputHeight);
	
	const auto maximumWorkItems = patchCount * options.Config.LightCount.Maximum;
	
	if (maximumWorkItems == 0u || maximumWorkItems > FDW_SD::Baseline::BATCH_CAPACITY) {
		m_sCaptureLastError = "configured resolution/light maximum exceeds the resident feature batch capacity";
		return false;
	}
	
	const auto estimatedBytesPerWorkItem =
		static_cast<std::uint64_t>(sizeof(FDW_SD::Work::NSWorkItem)) +
		FDW_SD::Features::PIXEL_CHANNEL_COUNT * FDW_SD::Features::PIXELS_PER_PATCH * 2u +
		FDW_SD::Features::LIGHT_CHANNEL_COUNT * 2u +
		FDW_SD::Features::PIXELS_PER_PATCH * 5u;
	
	const auto estimatedDatasetBytes = maximumWorkItems * estimatedBytesPerWorkItem * options.Config.SampleCount;
	const auto space = std::filesystem::space(options.DatasetRoot, ec);
	
	if (ec || space.available < estimatedDatasetBytes * 2u + 64u * 1024u * 1024u) {
		m_sCaptureLastError = ec ? "failed to query free dataset space: " + ec.message()
			: "insufficient free space for capture estimate plus safety reserve";
		return false;
	}
	
	m_xCaptureStartTime = std::chrono::steady_clock::now();
	m_pCaptureRTTargetProducer = std::make_unique<FDW_SD::GroundTruth::NeuralShadowRTTargetProducer>(
		renderer.GetDXRDevice(), renderer.GetDevice());
	
	if (!renderer.m_pShadowSystem->EnsureBlockerDistanceAtlasResource() ||
		!renderer.m_pShadowSystem->GetBlockerValidityAtlasResource()) {
		
		m_sCaptureLastError = "failed to allocate the required blocker and validity atlases";
		return false;
	}
	
	m_pInputCapture = std::make_unique<NeuralShadowInputCapture>();
	
	if (!m_pInputCapture->Initialize(renderer.GetDevice(), m_sCaptureLastError)) return false;
	
	return ConfigureSample(renderer);
}

bool NeuralShadowCaptureController::ConfigureSample(OfflineRenderer& renderer) {
	if (!m_xCaptureOptions) return false;
	
	auto& options = *m_xCaptureOptions;
	
	while (m_uCaptureSampleIndex < m_vCapturePlan.size()) {
		const auto& sample = m_vCapturePlan[m_uCaptureSampleIndex];
		NSNeuralShadowDataset::NSDatasetResult detail{};
		auto regeneration = NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
			options.DatasetRoot,
			options.Config.DatasetId,
			sample.RelativePath,
			sample.SampleId,
			sample.GenerationId,
			detail);
		
		if (regeneration == NSNeuralShadowDataset::NSRegenerationStatus::SkipValid) {
			FDW_SD::GroundTruth::NSRTTargetMetadataHeader targetMetadata{};
			const auto targetValidation = FDW_SD::GroundTruth::GroundTruthMetadata::LoadPublished(options.DatasetRoot, sample.RelativePath, &sample, targetMetadata);
		
			if (!targetValidation.Succeeded()) {
				regeneration = NSNeuralShadowDataset::NSRegenerationStatus::RegenerateInvalid;
			}
			else {
			
				if (!options.Resume) {
					m_sCaptureLastError = "valid sample already exists; use --resume to preserve/skip it";
					return false;
				}
				
				AppendJournal(sample, "skipped-valid", 0.0, 0u);
				++m_uCaptureSkipped;
				++m_uCaptureSampleIndex;
				continue;
			}
		}
		
		if (regeneration == NSNeuralShadowDataset::NSRegenerationStatus::Conflict) {
			m_sCaptureLastError = "capture resume conflict: " + detail.Message;
			return false;
		}
		break;
	}
	
	if (m_uCaptureSampleIndex >= m_vCapturePlan.size()) {
		Finish(renderer, true);
		return true;
	}

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	
	const auto& sample = m_vCapturePlan[m_uCaptureSampleIndex];
	renderer.m_pWorld = renderer.CreateEmptyWorld();
	
	const auto startsWith = [&sample](std::string_view prefix) {
		return sample.SceneAsset.rfind(prefix, 0u) == 0u;
	};
	
	bool addProceduralReceiver = startsWith("procedural:") || startsWith("mixed:");
	if (startsWith("world:")) {
		renderer.LoadWorld(sample.SceneAsset.substr(6u));
		
		for (auto* existingLight : renderer.m_pWorld->GetEntitiesByType<TLight>()) existingLight->Activate(false);
	
		for (auto* existingCamera : renderer.m_pWorld->GetEntitiesByType<TBaseCamera>()) existingCamera->Activate(false);
	}
	else if (startsWith("asset:") || startsWith("mixed:")) {
		const auto prefixSize = startsWith("asset:") ? 6u : 6u;
		
		renderer.m_pWorld->CreateScene(sample.SceneAsset.substr(prefixSize));
	}
	else if (!startsWith("procedural:")) {
		m_sCaptureLastError = "scene_asset entries must use procedural:, asset:, mixed:, or world: prefixes";
		
		return false;
	}

	auto* camera = renderer.m_pWorld->CreateDefaultCamera();
	camera->SetEyeAndLookAt(
		sample.Metadata.Camera.Eye.X,
		sample.Metadata.Camera.Eye.Y,
		sample.Metadata.Camera.Eye.Z,
		sample.Metadata.Camera.LookAt.X,
		sample.Metadata.Camera.LookAt.Y,
		sample.Metadata.Camera.LookAt.Z);
	camera->Activate(true);
	
	renderer.m_pCameraSystem->SetProjectionParameters(
		sample.Metadata.Camera.VerticalFovRadians,
		sample.Metadata.Camera.NearPlane,
		sample.Metadata.Camera.FarPlane);

	// Procedural and mixed profiles supply a stable receiver; asset/world profiles may supply their own.
	if (addProceduralReceiver) {
		auto* receiver = renderer.m_pWorld->CreateSimplePlane();
		receiver->SetPosition({ 0.0f, 0.0f, 0.0f });
		receiver->SetRotation({ -1.5707963f, 0.0f, 0.0f });
		receiver->SetScale({ 24.0f, 24.0f, 1.0f });
		receiver->SetDiffuse({ 0.65f, 0.65f, 0.65f, 1.0f });
	}

	for (const auto& geometry : sample.Geometry) {
		TMesh* mesh = nullptr;
		
		switch (static_cast<FDW_SD::Capture::NSCaptureGeometryKind>(geometry.Kind)) {
		case FDW_SD::Capture::NSCaptureGeometryKind::Plane: 
			mesh = renderer.m_pWorld->CreateSimplePlane(); 
			break;
		case FDW_SD::Capture::NSCaptureGeometryKind::Cube: 
			mesh = renderer.m_pWorld->CreateSimpleCube(); 
			break;
		case FDW_SD::Capture::NSCaptureGeometryKind::Sphere: 
			mesh = renderer.m_pWorld->CreateSimpleSphere(); 
			break;
		case FDW_SD::Capture::NSCaptureGeometryKind::Cone: mesh = 
			renderer.m_pWorld->CreateSimpleCone(); 
			break;
		case FDW_SD::Capture::NSCaptureGeometryKind::Cylinder: 
			mesh = renderer.m_pWorld->CreateSimpleCylinder(); 
			break;
		case FDW_SD::Capture::NSCaptureGeometryKind::Wall:

		case FDW_SD::Capture::NSCaptureGeometryKind::StairStep:

		case FDW_SD::Capture::NSCaptureGeometryKind::ThinBox:

		case FDW_SD::Capture::NSCaptureGeometryKind::IntersectingBox:
			mesh = renderer.m_pWorld->CreateSimpleCube();
			break;
		default: 
			break;
		}

		if (!mesh) continue;
		
		mesh->SetPosition({ geometry.Position.X, geometry.Position.Y, geometry.Position.Z });
		mesh->SetRotation({ geometry.RotationRadians.X, geometry.RotationRadians.Y, geometry.RotationRadians.Z });
		mesh->SetScale({ geometry.Scale.X, geometry.Scale.Y, geometry.Scale.Z });
		
		const auto channel = [&geometry](std::uint32_t shift) {
			return 0.2f + 0.75f * static_cast<float>((geometry.MaterialSeed >> shift) & 0xffu) / 255.0f;
		};
		
		if (auto* simpleMesh = dynamic_cast<TSimpleMesh*>(mesh)) {
			simpleMesh->SetDiffuse({ channel(0u), channel(8u), channel(16u), 1.0f });
		}
	}

	for (const auto& source : sample.Lights) {
		TLight* light = nullptr;
		
		switch (source.LightType) {
		case 0u: 
			light = renderer.m_pWorld->CreatePointLight(); 
			break;
		case 1u: 
			light = renderer.m_pWorld->CreateSpotLight(); 
			break;
		case 2u: 
			light = renderer.m_pWorld->CreateDirectionalLight(); 
			break;
		case 3u: 
			light = renderer.m_pWorld->CreateRectLight(); 
			break;
		default: 
			break;
		}
		
		if (!light) continue;
		
		light->SetLightColor({ source.Color.X, source.Color.Y, source.Color.Z });
		light->SetLightIntensity(source.Intensity);
		light->SetLightShadowSoftness(source.ShadowSoftness);
		
		if (auto* point = dynamic_cast<TPointLight*>(light)) {
			point->SetLightPosition({ source.Position.X, source.Position.Y, source.Position.Z });
			point->SetLightAttenuationRadius(source.AttenuationRadius);
			point->SetLightSourceRadius(source.SourceRadius);
		}
		else if (auto* spot = dynamic_cast<TSpotLight*>(light)) {
			spot->SetLightPosition({ source.Position.X, source.Position.Y, source.Position.Z });
			spot->SetLightDirection({ source.Direction.X, source.Direction.Y, source.Direction.Z });
			spot->SetLightAttenuationRadius(source.AttenuationRadius);
			spot->SetLightSourceRadius(source.SourceRadius);
			spot->SetLightInnerConeAngle(source.InnerConeAngle);
			spot->SetLightOuterConeAngle(source.OuterConeAngle);
		}
		else if (auto* directional = dynamic_cast<TDirectionalLight*>(light)) {
			directional->SetLightDirection({ source.Direction.X, source.Direction.Y, source.Direction.Z });
			directional->SetLightDirectionalAngularRadius(source.DirectionalAngularRadius);
		}
		else if (auto* rectangle = dynamic_cast<TRectLight*>(light)) {
			rectangle->SetLightPosition({ source.Position.X, source.Position.Y, source.Position.Z });
			rectangle->SetLightRectSize({ source.RectangleSize.X, source.RectangleSize.Y });
			rectangle->SetLightRotation({ source.RotationRadians.X, source.RotationRadians.Y, source.RotationRadians.Z });
		}

		light->ActivateLightComponent(true);
		light->ActivateShadowComponent(true);
	}

	HardShadowAtlasFilterSettings hard{};
	hard.Mode = HardShadowAtlasFilterMode::Nearest;
	hard.PCFKernelSize = 3u;
	hard.FilterRadius = 0.05f;
	hard.DepthRejectionSharpness = 0.01f;
	hard.NormalRejectionSharpness = 0.01f;
	hard.BlackLevel = 0.0f;
	hard.ShadowContrast = 1.0f;
	hard.NoiseScale = 0.0f;
	
	renderer.SetHardShadowAtlasFilterSettings(hard);
	renderer.ProcessNotifiesInWorld();
	
	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	
	m_xCaptureSampleStartTime = std::chrono::steady_clock::now();
	m_bCaptureSampleActive = true;
	m_uCaptureWarmupFramesRemaining = 1u;
	
	return true;
}

std::shared_ptr<FD3DW::ExecutionHandle> NeuralShadowCaptureController::Schedule(OfflineRenderer& renderer, const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies) {
	if (!m_xCaptureOptions || m_uCaptureSampleIndex >= m_vCapturePlan.size() || !m_pCaptureRTTargetProducer) return nullptr;
	
	const auto generation = m_vCapturePlan[m_uCaptureSampleIndex].GenerationId;
	
	m_bCaptureRecordSucceeded.store(false, std::memory_order_release);
	
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList5>>( D3D12_COMMAND_LIST_TYPE_DIRECT, [this, &renderer, generation](ID3D12GraphicsCommandList4* list) {
			NeuralShadowInputCapture::Inputs captureInputs{};
			captureInputs.WorkItems = renderer.m_pShadowSystem->GetNeuralWorkGenerator()->GetWorkItemsResource();
			captureInputs.WorkHeader = renderer.m_pShadowSystem->GetNeuralWorkGenerator()->GetDispatchHeaderBufferResource();
			captureInputs.ShadowMetadata = renderer.m_pShadowSystem->GetShadowMetadataResource();
			captureInputs.BlockerDistanceAtlas = renderer.m_pShadowSystem->EnsureBlockerDistanceAtlasResource();
			captureInputs.BlockerValidityAtlas = renderer.m_pShadowSystem->GetBlockerValidityAtlasResource();
			captureInputs.ScreenWidth = m_xCaptureOptions->Config.OutputWidth;
			captureInputs.ScreenHeight = m_xCaptureOptions->Config.OutputHeight;
			captureInputs.ShadowMetadataCount = renderer.GetShadowMetadataCount();
			
			std::string captureError;
			if (!m_pInputCapture || !m_pInputCapture->Record(list, captureInputs, captureError)) {
				m_sCaptureLastError = captureError;
				m_bCaptureRecordSucceeded.store(false, std::memory_order_release);
				return;
			}
			
			auto tlas = renderer.GetTLAS();
			
			FDW_SD::GroundTruth::NSRTTargetRecordInputs inputs{};
			inputs.TLAS = tlas.pResult.Get();
			inputs.Lights = renderer.GetLightsBuffer();
			inputs.WorkItems = renderer.m_pShadowSystem->GetNeuralWorkGenerator()->GetWorkItemsResource();
			inputs.WorkHeader = renderer.m_pShadowSystem->GetNeuralWorkGenerator()->GetDispatchHeaderBufferResource();
			inputs.PixelFeatures = renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetPixelFeaturesResource();
			inputs.Depth = renderer.GetCurrentDSV();
			inputs.Normal = renderer.m_pGBuffers[GBUFFER_NORMAL_LOCATION_IN_HEAP]->GetTexture();
			inputs.Visibility = renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetVisibilityResource();
			inputs.Validity = renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetValidityResource();
			inputs.OutputHeader = renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetOutputHeaderResource();
			
			dx::XMStoreFloat4x4(&inputs.InverseViewProjectionMatrix, dx::XMMatrixInverse(nullptr, dx::XMMatrixTranspose(renderer.GetViewProjectionMatrix())));
			inputs.QualitySettings = {
				NSRTQualityMode::OfflineGroundTruth,
				m_xCaptureOptions->Config.BaseSeed,
				m_xCaptureOptions->Config.TargetSamplesPerPixel,
				NSRTDenoisingPolicy::Disabled,
				generation
			};
			
			inputs.Geometry = renderer.GetRTGeometryCoverageState();
			inputs.GeometryInstances = renderer.GetRTGeometryInstancesResource();
			inputs.GeometryIndexBuffer = renderer.GetRTAlphaIndexBuffer();
			inputs.GeometryVertexBuffer = renderer.GetRTAlphaVertexBuffer();
			
			const auto accounting = renderer.GetRTGeometryAccounting();
			std::cerr << "[neural-shadow-geometry] total=" << accounting.TotalInstances
				<< " opaque=" << accounting.OpaqueInstances
				<< " alpha_tested=" << accounting.AlphaTestedInstances
				<< " unsupported_alpha=" << accounting.UnsupportedAlphaOrOpacityInstances
				<< " double_sided=" << accounting.DoubleSidedInstances
				<< " skinned=" << accounting.SkinnedInstances
				<< " triangles=" << accounting.TriangleCount
				<< " shared_alpha_buffers=" << accounting.SharedAlphaBuffersCompatible
				<< " missing_blas=" << inputs.Geometry.MissingBLASInstances
				<< " tlas_complete=" << inputs.Geometry.TLASComplete
				<< " canonical_eligible=" << inputs.Geometry.GroundTruthGeometryEligible
				<< '\n';
			
			inputs.GenerationId = generation;
			inputs.LightCount = static_cast<std::uint32_t>(std::max(0, renderer.GetLightsCount()));
			inputs.OutputWidth = m_xCaptureOptions->Config.OutputWidth;
			inputs.OutputHeight = m_xCaptureOptions->Config.OutputHeight;
			
			m_bCaptureRecordSucceeded.store( m_pCaptureRTTargetProducer->Record(list, inputs), std::memory_order_release);
		});
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, dependencies, true);
}

bool NeuralShadowCaptureController::Finalize(OfflineRenderer& renderer, const std::shared_ptr<FD3DW::ExecutionHandle>& captureHandle) {

	if (!captureHandle || !m_xCaptureOptions || m_uCaptureSampleIndex >= m_vCapturePlan.size()) return false;
	
	captureHandle->WaitForExecute();
	if (!m_bCaptureRecordSucceeded.load(std::memory_order_acquire)) {
		m_sCaptureLastError = "DXR target recorder rejected the live frame inputs";
		return false;
	}

	auto readBuffers = [this, &renderer](
		std::span<const FD3DW::BufferReadbackSource> sources,
		std::vector<std::vector<std::byte>>& slices,
		std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> dependencies) -> bool {
		
		std::unique_ptr<FD3DW::BufferReadbackBatch> batch;
		std::string error;
		if (!FD3DW::BufferReadbackBatch::Create(renderer.GetDevice(), sources, batch, error)) {
			m_sCaptureLastError = error;
			return false;
		}

		auto recordSucceeded = std::make_shared<std::atomic<bool>>(false);
		auto recordError = std::make_shared<std::string>();
		auto* batchPtr = batch.get();
		
		auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			[batchPtr, recordSucceeded, recordError](ID3D12GraphicsCommandList* list) {
				std::string localError;
				const auto ok = batchPtr->RecordCopies(list, localError);
				*recordError = std::move(localError);
				recordSucceeded->store(ok, std::memory_order_release);
			});

		auto handle = GlobalRenderThreadManager::GetInstance()->Submit(recipe, std::move(dependencies), true);
		handle->WaitForExecute();
		
		if (!recordSucceeded->load(std::memory_order_acquire)) {
			m_sCaptureLastError = *recordError;
			return false;
		}
		
		if (!batch->ReadAfterCompletion(handle->GetFence(), handle->GetFenceValue(), slices, error)) {
			m_sCaptureLastError = error;
			return false;
		}
		
		return true;
	};

	const FD3DW::BufferReadbackSource headerSources[] = {
		{ renderer.m_pShadowSystem->GetNeuralWorkGenerator()->GetDispatchHeaderBufferResource(), 0u, sizeof(FDW_SD::Work::NSWorkDispatchHeader) },
		{ renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetOutputHeaderResource(), 0u, sizeof(FDW_SD::Output::NSOutputHeader) }
	};
	
	std::vector<std::vector<std::byte>> headerSlices;
	if (!readBuffers(headerSources, headerSlices, { captureHandle })) return false;
	
	FDW_SD::Work::NSWorkDispatchHeader workHeader{};
	FDW_SD::Output::NSOutputHeader outputHeader{};
	
	std::memcpy(&workHeader, headerSlices[0].data(), sizeof(workHeader));
	std::memcpy(&outputHeader, headerSlices[1].data(), sizeof(outputHeader));
	
	const auto& sample = m_vCapturePlan[m_uCaptureSampleIndex];
	
	if (!IsDatasetPublishable(workHeader) ||
		!IsCompleteOutputForViewport(outputHeader, workHeader, sample.GenerationId,
			sample.Metadata.OutputWidth, sample.Metadata.OutputHeight) ||
		outputHeader.Producer != static_cast<std::uint32_t>(FDW_SD::Output::NSOutputProducer::RayTracedReference)) {
		m_sCaptureLastError = "live GPU work/output headers are incomplete or incompatible";
		return false;
	}
	
	if (workHeader.WorkItemCount > FDW_SD::Baseline::BATCH_CAPACITY) {
		m_sCaptureLastError = "capture exceeds the resident feature batch capacity";
		return false;
	}

	const auto workBytes = static_cast<std::uint64_t>(workHeader.WorkItemCount) * sizeof(FDW_SD::Work::NSWorkItem);
	const auto pixelBytes = static_cast<std::uint64_t>(workHeader.WorkItemCount) * FDW_SD::Features::PIXEL_CHANNEL_COUNT * FDW_SD::Features::PIXELS_PER_PATCH * sizeof(std::uint16_t);
	const auto lightBytes = static_cast<std::uint64_t>(workHeader.WorkItemCount) * FDW_SD::Features::LIGHT_CHANNEL_COUNT * sizeof(std::uint16_t);
	const auto blockerBytes = static_cast<std::uint64_t>(workHeader.WorkItemCount) * NSOfflineConfig::NeuralShadows::InputCaptureValuesPerWork * sizeof(std::uint16_t);
	const auto inputValidityBytes = static_cast<std::uint64_t>(workHeader.WorkItemCount) * NSOfflineConfig::NeuralShadows::InputCaptureValuesPerWork;
	
	std::vector<FD3DW::BufferReadbackSource> dataSources = {
		{ renderer.m_pShadowSystem->GetNeuralWorkGenerator()->GetWorkItemsResource(), 0u, workBytes },
		{ renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetPixelFeaturesResource(), 0u, pixelBytes },
		{ renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetLightFeaturesResource(), 0u, lightBytes },
		{ renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetVisibilityResource(), 0u, outputHeader.VisibilityByteCount },
		{ renderer.m_pShadowSystem->GetNeuralBaselinePass()->GetValidityResource(), 0u, outputHeader.ValidityByteCount },
		{ m_pInputCapture->GetBlockerValues(), 0u, blockerBytes },
		{ m_pInputCapture->GetInputValidity(), 0u, inputValidityBytes }
	};
	
	std::vector<std::vector<std::byte>> slices;
	
	if (!readBuffers(dataSources, slices, {})) return false;
	
	std::vector<std::byte> hardInput(static_cast<std::size_t>(workHeader.OutputValueCount) * sizeof(std::uint16_t));
	
	for (std::uint32_t work = 0u; work < workHeader.WorkItemCount; ++work) {
		for (std::uint32_t pixel = 0u; pixel < FDW_SD::Features::PIXELS_PER_PATCH; ++pixel) {
			const auto sourceHalf = (static_cast<std::uint64_t>(work) * FDW_SD::Features::PIXEL_CHANNEL_COUNT + static_cast<std::uint32_t>(FDW_SD::Features::NSPixelFeatureChannel::HardVisibility)) * FDW_SD::Features::PIXELS_PER_PATCH + pixel;
			const auto destinationHalf = static_cast<std::uint64_t>(work) * FDW_SD::Features::PIXELS_PER_PATCH + pixel;
			std::memcpy(hardInput.data() + destinationHalf * sizeof(std::uint16_t), slices[1].data() + sourceHalf * sizeof(std::uint16_t), sizeof(std::uint16_t));
		}
	}

	std::vector<FDW_SD::Capture::NSCaptureOwnedBlock> blocks;
	if (!BuildNeuralOutputBlocks(sample,workHeader,slices[0],slices[1],slices[2],hardInput,slices[3],slices[4],slices[5],slices[6],blocks,m_sCaptureLastError)) return false;

	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::WorkHeader, FDW_SD::Work::DATA_LAYOUT_VERSION, 1u, headerSlices[0] });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::WorkItems, FDW_SD::Work::DATA_LAYOUT_VERSION, workHeader.WorkItemCount, slices[0] });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::PixelFeatures, FDW_SD::Features::FEATURE_LAYOUT_VERSION, static_cast<std::uint64_t>(workHeader.OutputValueCount) * FDW_SD::Features::PIXEL_CHANNEL_COUNT, slices[1] });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::LightFeatures, FDW_SD::Features::FEATURE_LAYOUT_VERSION, static_cast<std::uint64_t>(workHeader.WorkItemCount) * FDW_SD::Features::LIGHT_CHANNEL_COUNT, slices[2] });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::HardInputVisibility, 1u, workHeader.OutputValueCount, std::move(hardInput) });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::RTTargetVisibility, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, outputHeader.OutputValueCount, slices[3] });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::TargetValidity, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, outputHeader.OutputValueCount, slices[4] });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::OutputHeader, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, 1u, headerSlices[1] });

	FDW_SD::GroundTruth::NSRTTargetCaptureEvidence evidence{};
	evidence.EngineRevision = "FDW neural-shadow offline renderer";
	evidence.QualitySettings = {
		NSRTQualityMode::OfflineGroundTruth,
		m_xCaptureOptions->Config.BaseSeed,
		m_xCaptureOptions->Config.TargetSamplesPerPixel,
		NSRTDenoisingPolicy::Disabled,
		sample.GenerationId
	};

	evidence.Geometry = renderer.GetRTGeometryCoverageState();
	
	FDW_SD::GroundTruth::NSRTTargetValidationStats validationStats{};
	
	auto result = FDW_SD::GroundTruth::GroundTruthMetadata::ValidateBlocks(sample, blocks, evidence, validationStats);
	if (!result.Succeeded()) {
		m_sCaptureLastError = result.Message;
		return false;
	}
	
	std::vector<std::byte> targetMetadata;
	
	result = FDW_SD::GroundTruth::GroundTruthMetadata::Build(sample, evidence, validationStats, targetMetadata);
	if (!result.Succeeded()) {
		m_sCaptureLastError = result.Message;
		return false;
	}
	
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata, FDW_SD::Capture::CAPTURE_METADATA_VERSION, 1u, sample.MetadataBytes });
	blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::RTTargetMetadata, FDW_SD::GroundTruth::RT_TARGET_METADATA_VERSION, 1u, std::move(targetMetadata) });
	
	const auto publication = DatasetWriter.Publish(*m_xCaptureOptions, sample, blocks);
	if (!publication.Succeeded()) {
		m_sCaptureLastError = publication.Message;
		return false;
	}
	
	std::error_code ec;
	const auto sampleBytes = std::filesystem::file_size( m_xCaptureOptions->DatasetRoot / sample.RelativePath, ec);
	if (!ec) {
		m_uCaptureOutputBytes += sampleBytes;
		DatasetWriter.PublishedBytes += sampleBytes;
	}
	
	const auto sampleElapsed = std::chrono::duration<double>( std::chrono::steady_clock::now() - m_xCaptureSampleStartTime).count();
	
	AppendJournal(sample, "generated-valid", sampleElapsed, ec ? 0u : sampleBytes);
	
	m_bCaptureSampleActive = false;
	++m_uCaptureGenerated;
	++m_uCaptureSampleIndex;
	
	const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_xCaptureStartTime).count();
	const auto completed = m_uCaptureGenerated + m_uCaptureSkipped;
	const auto remaining = m_vCapturePlan.size() - completed;
	const auto eta = completed == 0u ? 0.0 : elapsed / completed * remaining;
	
	std::cout << "[neural-shadow-capture] " << completed << '/' << m_vCapturePlan.size()
		<< " generated=" << m_uCaptureGenerated << " skipped=" << m_uCaptureSkipped
		<< " eta_seconds=" << std::fixed << std::setprecision(1) << eta << '\n';
	
	return ConfigureSample(renderer);
}

void NeuralShadowCaptureController::WriteReport(bool succeeded, const std::string& message) const {
	if (!m_xCaptureOptions) return;
	
	auto json = [](std::string_view value) {
		std::string encoded;
		
		for (const auto character : value) {
			switch (character) {
			case '\\': 
				encoded += "\\\\"; 
				break;
			case '"': 
				encoded += "\\\""; 
				break;
			case '\n': 
				encoded += "\\n"; 
				break;
			case '\r': 
				encoded += "\\r"; 
				break;
			case '\t': 
				encoded += "\\t"; 
				break;
			default: 
				encoded += character; 
				break;
			}
		}

		return encoded;
	};
	
	const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_xCaptureStartTime).count();
	
	std::ofstream stream(m_xCaptureOptions->DatasetRoot / "capture_report.json", std::ios::trunc);
	if (!stream) return;
	
	stream << "{\n"
		<< "  \"format\": \"NS_LIVE_CAPTURE_REPORT\",\n"
		<< "  \"succeeded\": " << (succeeded ? "true" : "false") << ",\n"
		<< "  \"planned_samples\": " << m_vCapturePlan.size() << ",\n"
		<< "  \"generated_samples\": " << m_uCaptureGenerated << ",\n"
		<< "  \"skipped_samples\": " << m_uCaptureSkipped << ",\n"
		<< "  \"resolution\": [" << m_xCaptureOptions->Config.OutputWidth << ", "
		<< m_xCaptureOptions->Config.OutputHeight << "],\n"
		<< "  \"spp\": " << m_xCaptureOptions->Config.TargetSamplesPerPixel << ",\n"
		<< "  \"sample_bytes\": " << m_uCaptureOutputBytes << ",\n"
		<< "  \"elapsed_seconds\": " << std::fixed << std::setprecision(6) << elapsed << ",\n"
		<< "  \"engine_revision\": \"FDW neural-shadow offline renderer\",\n"
		<< "  \"message\": \"" << json(message) << "\"\n"
		<< "}\n";
}

void NeuralShadowCaptureController::AppendJournal(const FDW_SD::Capture::NSCaptureSamplePlan& sample,std::string_view status,double elapsedSeconds,std::uint64_t byteSize,std::string_view message) const {
	if (!m_xCaptureOptions) return;
	
	const auto path = m_xCaptureOptions->DatasetRoot / "capture_journal.csv";
	
	std::error_code ec;
	const auto writeHeader = !std::filesystem::exists(path, ec) || std::filesystem::file_size(path, ec) == 0u;
	
	std::ofstream stream(path, std::ios::app);
	if (!stream) return;
	
	auto csv = [](std::string_view value) {
		std::string encoded = "\"";
		for (const auto character : value) {
			if (character == '"') encoded += '"';
			encoded += character;
		}
		encoded += '"';
		return encoded;
	};
	
	if (writeHeader) {
		stream << "sample_id,generation_id,relative_path,scene,status,resolution,spp,bytes,elapsed_seconds,finished_unix_seconds,message\n";
	}
	
	const auto finished = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	stream << sample.SampleId << ',' << sample.GenerationId << ',' << csv(sample.RelativePath.generic_string()) << ','
		<< csv(sample.SceneAsset) << ',' << csv(status) << ','
		<< csv(std::to_string(sample.Metadata.OutputWidth) + "x" + std::to_string(sample.Metadata.OutputHeight)) << ','
		<< m_xCaptureOptions->Config.TargetSamplesPerPixel << ',' << byteSize << ','
		<< std::fixed << std::setprecision(6) << elapsedSeconds << ',' << finished << ',' << csv(message) << '\n';
	
	stream.flush();
}

void NeuralShadowCaptureController::Finish(OfflineRenderer& renderer, bool succeeded, std::string message) {
	if (m_bCaptureFinished) return;
	
	m_bCaptureFinished = true;
	m_iCaptureExitCode = succeeded ? 0 : 1;
	
	if (!succeeded && message.empty()) message = "neural-shadow capture failed";
	
	if (!succeeded && m_bCaptureSampleActive && m_uCaptureSampleIndex < m_vCapturePlan.size()) {
		const auto elapsed = std::chrono::duration<double>(
			std::chrono::steady_clock::now() - m_xCaptureSampleStartTime).count();
		AppendJournal(m_vCapturePlan[m_uCaptureSampleIndex], "failed", elapsed, 0u, message);
		m_bCaptureSampleActive = false;
	}
	
	if (!succeeded) std::cerr << "[neural-shadow-capture] error: " << message << '\n';
	
	WriteReport(succeeded, message);
	renderer.RequestExit();
}
