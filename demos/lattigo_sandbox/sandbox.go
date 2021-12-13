package main

import (
    "fmt"
    "math"

    "github.com/ldsec/lattigo/v2/ckks"
    "github.com/ldsec/lattigo/v2/rlwe"
    "github.com/ldsec/lattigo/v2/utils"
    "github.com/ldsec/lattigo/v2/ckks/bootstrapping"
    "github.com/ldsec/lattigo/v2/ckks/advanced"
)

func main() {

    test()

    // multiplicationChain()

    // myBootstrapping()

    // vectorDiff()

    // levelError()
}

func test() {

    params := getSimpleParams()
    keygen := ckks.NewKeyGenerator(params)
    sk, pk := keygen.GenKeyPair()
    rlk := keygen.GenRelinearizationKey(sk, 2)
    encoder := ckks.NewEncoder(params)
    encryptor := ckks.NewEncryptor(params, pk)
    decryptor := ckks.NewDecryptor(params, sk)
    evaluator := ckks.NewEvaluator(params, rlwe.EvaluationKey{Rlk: rlk})

    /*
    btpParams := getSimpleBootstrappingParams()
    fmt.Printf("Generate bootstrapping keys ... ")
    rotations := btpParams.RotationsForBootstrapping(params.LogN(), params.LogSlots())
    rotkeys := keygen.GenRotationKeysForRotations(rotations, true, sk)
    btp, err := bootstrapping.NewBootstrapper(params, btpParams, rlwe.EvaluationKey{Rlk: rlk, Rtks: rotkeys})
    if err != nil {
        panic(err)
    }
    fmt.Printf("Done \n")
    */

    fmt.Printf("Q0: %x \n", params.QiFloat64(0))

    // Plaintexts
    values := make([]complex128, params.Slots())
    factors := make([]complex128, params.Slots())

    for i := range values {
        values[i] = complex(1, 0)
        factors[i] = complex(2, 0)
    }

    // Encode and Encrypt
    ct   := encryptor.EncryptNew(encoder.EncodeNew(values , params.LogSlots()))
    ct_f := encryptor.EncryptNew(encoder.EncodeNew(factors, params.LogSlots()))

    _ = evaluator
    _ = ct_f
    _ = decryptor
    _ = ct


    /*
    // Bootstrap # 1
    fmt.Printf("Bootstrapping ... ")
    ct_btp1 := btp.Bootstrapp(ct)
    fmt.Printf("Done --> Level: %d, Scale: %f \n", ct_btp1.Level(), math.Log2(ct_btp1.Scale))

    z := encoder.Decode(decryptor.DecryptNew(ct_btp1), params.LogSlots())
    fmt.Printf("ct_new[0] = %f \n", real(z[0]))
    */

}

func multiplicationChain() {

    params := getSimpleParams()

    keygen := ckks.NewKeyGenerator(params)
    sk, pk := keygen.GenKeyPair()
    rlk := keygen.GenRelinearizationKey(sk, 2)
    encoder := ckks.NewEncoder(params)
    encryptor := ckks.NewEncryptor(params, pk)
    decryptor := ckks.NewDecryptor(params, sk)
    evaluator := ckks.NewEvaluator(params, rlwe.EvaluationKey{Rlk: rlk})

    btpParams := getSimpleBootstrappingParams()
    fmt.Printf("Generate bootstrapping keys ... ")
    rotations := btpParams.RotationsForBootstrapping(params.LogN(), params.LogSlots())
    rotkeys := keygen.GenRotationKeysForRotations(rotations, true, sk)
    btp, err := bootstrapping.NewBootstrapper(params, btpParams, rlwe.EvaluationKey{Rlk: rlk, Rtks: rotkeys})
    if err != nil {
        panic(err)
    }
    fmt.Printf("Done \n")

    _ = btp

    // Plaintexts
    values := make([]complex128, params.Slots())
    factors := make([]complex128, params.Slots())

    for i := range values {
        values[i] = complex(1, 0)
        factors[i] = complex(2, 0)
    }

    // Encode and Encrypt
    ct   := encryptor.EncryptNew(encoder.EncodeNew(values , params.LogSlots()))
    ct_f := encryptor.EncryptNew(encoder.EncodeNew(factors, params.LogSlots()))


    // Mult Chain
    fmt.Printf("Level: %d, %d; ct[0] = %7.3f, scale: %.1f \n", ct.Level(), ct_f.Level(), real(values[0]), math.Log2(ct.Scale))
    var z []complex128
    for ct.Level() > 0 {
        evaluator.MulRelin(ct, ct_f, ct)
        if err := evaluator.Rescale(ct, params.Scale(), ct); err != nil {
            panic(err)
        }
        z = encoder.Decode(decryptor.DecryptNew(ct), params.LogSlots())
        fmt.Printf("Level: %d, %d; ct[0] = %7.3f, scale: %.1f \n", ct.Level(), ct_f.Level(), real(z[0]), math.Log2(ct.Scale))
    }

}

func getSimpleParams() (ckks.Parameters) {
    params, err := ckks.NewParametersFromLiteral(ckks.ParametersLiteral{
        LogN: 14,
        LogQ: []int{60,40,40,40,40,40,40,40,40},
        LogP: []int{60},
        Sigma: rlwe.DefaultSigma,
        LogSlots: 9,
        Scale: float64(1<<40),
    })
    if err != nil {
        panic(err)
    }

    return params

}

func getSimpleBootstrappingParams() (bootstrapping.Parameters) {

    params := bootstrapping.Parameters{
        H: 192,
        SlotsToCoeffsParameters: advanced.EncodingMatrixLiteral{
			LinearTransformType: advanced.SlotsToCoeffs,
			LevelStart:          3,
			BSGSRatio:           2.0,
			BitReversed:         false,
			ScalingFactor: [][]float64{
				{1073741823.9998779, 1073741823.9998779},
			},
		},
		EvalModParameters: advanced.EvalModLiteral{
			Q:             0x1fff90001,
			LevelStart:    11,
			SineType:      advanced.Cos1,
			MessageRatio:  256.0,
			K:             40,
			SineDeg:       63,
			DoubleAngle:   2,
			ArcSineDeg:    0,
			ScalingFactor: 1 << 40,
		},
		CoeffsToSlotsParameters: advanced.EncodingMatrixLiteral{
			LinearTransformType: advanced.CoeffsToSlots,
			LevelStart:          13,
			BSGSRatio:           2.0,
			BitReversed:         false,
			ScalingFactor: [][]float64{
				{0x1fffffff50001},
				{0x1ffffffea0001},
			},
		},
    }

    return params
}

func getCkksParams(paramSet int) (ckks.Parameters) {
    if paramSet < 0 || paramSet > 4 {
        panic("Invalid CKKS Param Set Index")
    }

    params, err := ckks.NewParametersFromLiteral(bootstrapping.DefaultCKKSParameters[paramSet])
    if err != nil {
        panic(err)
    }

    return params
}

func getBootstrappingParams(paramSet int) (bootstrapping.Parameters) {
    if paramSet < 0 || paramSet > 4 {
        panic("Invalid Bootstrapping Param Set Index")
    }

    return bootstrapping.DefaultParameters[paramSet]
}


func myBootstrapping() {

    params := getCkksParams(4)
	btpParams := getBootstrappingParams(4)

    fmt.Println()
	fmt.Printf("CKKS parameters: logN = %d, logSlots = %d, h = %d, logQP = %d, levels = %d, scale= 2^%f, sigma = %f \n", params.LogN(), params.LogSlots(), btpParams.H, params.LogQP(), params.QCount(), math.Log2(params.Scale()), params.Sigma())

    fmt.Printf("Generate keys ... ")
    keygen := ckks.NewKeyGenerator(params)
    sk, pk := keygen.GenKeyPairSparse(btpParams.H)
    rlk := keygen.GenRelinearizationKey(sk, 2)
    encoder := ckks.NewEncoder(params)
    encryptor := ckks.NewEncryptor(params, pk)
    decryptor := ckks.NewDecryptor(params, sk)
    evaluator := ckks.NewEvaluator(params, rlwe.EvaluationKey{Rlk: rlk})
    fmt.Printf("Done \n")

    fmt.Printf("Generate bootstrapping keys ... ")
    rotations := btpParams.RotationsForBootstrapping(params.LogN(), params.LogSlots())
    rotkeys := keygen.GenRotationKeysForRotations(rotations, true, sk)
    btp, err := bootstrapping.NewBootstrapper(params, btpParams, rlwe.EvaluationKey{Rlk: rlk, Rtks: rotkeys})
    if err != nil {
        panic(err)
    }
    fmt.Printf("Done \n")

    values := make([]complex128, params.Slots())
    for i := range values {
        values[i] = complex(1,0)
    }

    pt := encoder.EncodeNew(values, params.LogSlots())

    ct := encryptor.EncryptNew(pt)

    fmt.Printf("ct[0] = %f --> Level: %d, Scale: %f \n", values[0], ct.Level(), ct.Scale)

    // Bootstrapping
    fmt.Printf("Bootstrapping ... ")
    ct_new := btp.Bootstrapp(ct)
    fmt.Printf("Done --> Level: %d, Scale: %f \n", ct_new.Level(), ct_new.Scale)

    z := encoder.Decode(decryptor.DecryptNew(ct_new), params.LogSlots())
    fmt.Printf("ct_new[0] = %f \n", real(z[0]))

    // Square
    evaluator.Power(ct_new, 2, ct_new)
    z = encoder.Decode(decryptor.DecryptNew(ct_new), params.LogSlots())
    fmt.Printf("Squared: ct_new[0] = %f --> Level: %d, Scale: %f \n", real(z[0]), ct_new.Level(), ct_new.Scale)

    // Re-Set Scale to params.Scale, consumes one level
    evaluator.SetScale(ct_new, params.Scale())
    fmt.Printf("Rescaled --> Level: %d, Scale: %f \n", ct_new.Level(), ct_new.Scale)


    // Re-Bootstrap
    fmt.Printf("Bootstrapping ... ")
    ct_2nd := btp.Bootstrapp(ct_new)
    fmt.Printf("Done --> Level: %d, Scale: %f \n", ct_2nd.Level(), ct_2nd.Scale)

    z = encoder.Decode(decryptor.DecryptNew(ct_2nd), params.LogSlots())
    fmt.Printf("ct_2nd[0] = %f \n", real(z[0]))


}

func vectorDiff() {
    // Init Parameters
    params := getSimpleParams()

    keygen := ckks.NewKeyGenerator(params)
    sk, pk := keygen.GenKeyPair()
    rlk := keygen.GenRelinearizationKey(sk, 2)
    encoder := ckks.NewEncoder(params)
    encryptor := ckks.NewEncryptor(params, pk)
    decryptor := ckks.NewDecryptor(params, sk)
    evaluator := ckks.NewEvaluator(params, rlwe.EvaluationKey{Rlk: rlk})

    // Plaintexts
    v1 := make([]complex128, params.Slots())
    v2 := make([]complex128, params.Slots())

    for i := range v1 {
        v1[i] = complex(utils.RandFloat64(0, 4), 0)
        v2[i] = complex(utils.RandFloat64(0, 4), 0)
    }

    // Encode and Encrypt
    p1 := ckks.NewPlaintext(params, params.MaxLevel(), params.Scale())
    p2 := ckks.NewPlaintext(params, params.MaxLevel(), params.Scale())

    encoder.Encode(p1, v1, params.LogSlots())
    encoder.Encode(p2, v2, params.LogSlots())

    ct1 := encryptor.EncryptNew(p1)
    ct2 := encryptor.EncryptNew(p2)

    // Subtract: v1 - v2
    ct_res := evaluator.SubNew(ct1, ct2)

    z := encoder.Decode(decryptor.DecryptNew(ct_res), params.LogSlots())
    for i := range z {
        fmt.Printf("%.3f - %.3f = %.3f \n", real(v1[i]), real(v2[i]), real(z[i]))
    }
}

func levelError() {
    // Init Parameters
    var err error
    params, err := ckks.NewParametersFromLiteral(ckks.ParametersLiteral{
        LogN: 12,
        LogQ: []int{55,40,40},
        LogP: []int{45,45},
        Sigma: rlwe.DefaultSigma,
        LogSlots: 10,
        Scale: float64(1<<40),
    })
    if err != nil {
        panic(err)
    }

    keygen := ckks.NewKeyGenerator(params)
    sk, pk := keygen.GenKeyPair()
    rlk := keygen.GenRelinearizationKey(sk, 2)
    encoder := ckks.NewEncoder(params)
    encryptor := ckks.NewEncryptor(params, pk)
    decryptor := ckks.NewDecryptor(params, sk)
    evaluator := ckks.NewEvaluator(params, rlwe.EvaluationKey{Rlk: rlk})

    // Plaintexts
    v1 := make([]complex128, params.Slots())

    for i := range v1 {
        v1[i] = complex(utils.RandFloat64(-4, 4), 0)
    }

    // Encode and Encrypt
    p1 := ckks.NewPlaintext(params, params.MaxLevel(), params.Scale())

    encoder.Encode(p1, v1, params.LogSlots())

    ct1 := encryptor.EncryptNew(p1)

    var z []complex128
    // Square 
    for ct1.Level() > 0 {
        evaluator.Power(ct1, 2, ct1)

        z = encoder.Decode(decryptor.DecryptNew(ct1), params.LogSlots())
        fmt.Printf("Level: %d, ct[0] = %f \n", ct1.Level(), real(z[0]))
    }

    // Square at level 0 -> shouldn't be possible
    // yep, output is ...
    // panic: cannot Rescale: input Ciphertext already at level 0
    /*
    evaluator.Power(ct1, 2, ct1)
    */

}

func printDebug(params ckks.Parameters, ciphertext *ckks.Ciphertext, valuesWant []complex128, decryptor ckks.Decryptor, encoder ckks.Encoder) (valuesTest []complex128) {

	valuesTest = encoder.Decode(decryptor.DecryptNew(ciphertext), params.LogSlots())

	fmt.Println()
	fmt.Printf("Level: %d (logQ = %d)\n", ciphertext.Level(), params.LogQLvl(ciphertext.Level()))
	fmt.Printf("Scale: 2^%f\n", math.Log2(ciphertext.Scale))
	fmt.Printf("ValuesTest: %6.10f %6.10f %6.10f %6.10f...\n", valuesTest[0], valuesTest[1], valuesTest[2], valuesTest[3])
	fmt.Printf("ValuesWant: %6.10f %6.10f %6.10f %6.10f...\n", valuesWant[0], valuesWant[1], valuesWant[2], valuesWant[3])

	precStats := ckks.GetPrecisionStats(params, encoder, nil, valuesWant, valuesTest, params.LogSlots(), 0)

	fmt.Println(precStats.String())
	fmt.Println()

	return
}
