// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#pragma once

#include "VaOceanPluginPrivatePCH.h"
#include "VaOceanSimulatorComponent.generated.h"

#define PAD16(n) (((n)+15)/16*16)

/** Phillips spectrum configuration */
USTRUCT()
struct FOceanData
{
	GENERATED_USTRUCT_BODY()

	/** The size of displacement map. Must be power of 2. */
	UPROPERTY(BlueprintReadOnly, Category = Ocean)
	int32 DispMapDimension;

	/** The side length (world space) of square patch. Typical value is 1000 ~ 2000. */
	UPROPERTY(EditDefaultsOnly, Category = Ocean)
	float PatchLength;

	/** Adjust the time interval for simulation (controls the simulation speed) */
	UPROPERTY(EditDefaultsOnly, Category = Ocean)
	float TimeScale;

	/** Amplitude for transverse wave. Around 1.0 (not the world space height). */
	UPROPERTY(EditDefaultsOnly, Category = Ocean)
	float WaveAmplitude;

	/** Wind direction. Normalization not required */
	UPROPERTY(EditDefaultsOnly, Category = Ocean)
	FVector2D WindDirection;

	/** The bigger the wind speed, the larger scale of wave crest. But the wave scale can be no larger than PatchLength. 
		Around 100 ~ 1000 */
	UPROPERTY(EditDefaultsOnly, Category = Ocean)
	float WindSpeed;

	/** This value damps out the waves against the wind direction. Smaller value means higher wind dependency. */
	UPROPERTY(EditDefaultsOnly, Category = Ocean)
	float WindDependency;

	/** The amplitude for longitudinal wave. Higher value creates pointy crests. Must be positive. */
	UPROPERTY(EditDefaultsOnly, Category = Ocean)
	float ChoppyScale;

	/** Defaults */
	FOceanData()
	{
		DispMapDimension = 512;		// Not editable because of FFT shader config
		PatchLength = 2000.0f;
		TimeScale = 0.8f;
		WaveAmplitude = 0.35f;
		WindDirection = FVector2D(0.8f, 0.6f);
		WindSpeed = 600.0f;
		WindDependency = 0.07f;
		ChoppyScale = 1.3f;
	}
};

/**
 * Renders normals and heightmap from Phillips spectrum
 */
UCLASS(ClassGroup=Environment, editinlinenew, meta=(BlueprintSpawnableComponent))
class UVaOceanSimulatorComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** Render target for normal map that can be used by the editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OceanSpectrum)
	class UTextureRenderTarget2D* NormalsTarget;

	/** Render target for height map that can be used by the editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OceanSpectrum)
	class UTextureRenderTarget2D* HeightTarget;

protected:
	/** Ocean spectrum data */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FOceanData OceanConfig;

	/** Initialize the vector field */
	void InitHeightMap(FOceanData& Params, TResourceArray<FVector2D>& out_h0, TResourceArray<float>& out_omega);

	void CreateBufferAndUAV(FResourceArrayInterface* Data, uint32 byte_width, uint32 byte_stride,
		FStructuredBufferRHIRef& ppBuffer, FUnorderedAccessViewRHIRef& ppUAV, FShaderResourceViewRHIRef& ppSRV);

public:
	// Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	// End UActorComponent Interface

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostInitProperties() OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	/** Update normals and heightmap from spectrum */
	void UpdateContent();
	void UpdateDisplacementMap(float WorldTime);

protected:

	//////////////////////////////////////////////////////////////////////////
	// Parameters that will be send to rendering thread

	FUpdateSpectrumCSImmutable UpdateSpectrumCSImmutableParams;


	//////////////////////////////////////////////////////////////////////////
	// Spectrum simulation data

	// Initial height field H(0) generated by Phillips spectrum & Gauss distribution.
	FStructuredBufferRHIRef m_pBuffer_Float2_H0;
	FUnorderedAccessViewRHIRef m_pUAV_H0;
	FShaderResourceViewRHIRef m_pSRV_H0;

	// Angular frequency
	FStructuredBufferRHIRef m_pBuffer_Float_Omega;
	FUnorderedAccessViewRHIRef m_pUAV_Omega;
	FShaderResourceViewRHIRef m_pSRV_Omega;

	// Height field H(t), choppy field Dx(t) and Dy(t) in frequency domain, updated each frame.
	FStructuredBufferRHIRef m_pBuffer_Float2_Ht;
	FUnorderedAccessViewRHIRef m_pUAV_Ht;
	FShaderResourceViewRHIRef m_pSRV_Ht;

	// Height & choppy buffer in the space domain, corresponding to H(t), Dx(t) and Dy(t)
	FStructuredBufferRHIRef m_pBuffer_Float_Dxyz;
	FUnorderedAccessViewRHIRef m_pUAV_Dxyz;
	FShaderResourceViewRHIRef m_pSRV_Dxyz;

	// FFT wrap-up
	FRadixPlan512 FFTPlan;

};
