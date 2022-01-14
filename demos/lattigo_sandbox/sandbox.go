package main

import (
    "fmt"
    "math"
    "time"
    "os"
    "strconv"

    "github.com/ldsec/lattigo/v2/ckks"
    "github.com/ldsec/lattigo/v2/rlwe"
    "github.com/ldsec/lattigo/v2/utils"
    "github.com/ldsec/lattigo/v2/ckks/bootstrapping"
    "github.com/ldsec/lattigo/v2/ckks/advanced"
)

func main() {
    args := os.Args[1:]

    if len(args) < 1 {
        panic("Invalid number of arguments")
    }

    paramSet, err := strconv.Atoi(args[0])
    if err != nil {
        panic(err)
    }

    if len(args) >=2 {
        slots, err := strconv.Atoi(args[1])
        if err != nil {
            panic(err)
        }
        bootstrapTime(paramSet, slots)
    } else {
        bootstrapTime(paramSet, 0)
    }

    // customBootstrapping()

    // printQs()

    // multiplicationChain()

    // myBootstrapping()

    // vectorDiff()

    // levelError()
}


func bootstrapTime(paramSet int, logSlots int) {
    var start time.Time

    i := paramSet

    fmt.Printf("Parameter Set %d: \n", i)
    fmt.Printf("Key Generation ... ")

    ckksParams := getCkksParamsWithSlots(i, logSlots)
    // ckksParams := getCkksParams(4)

    fmt.Printf("with logSlots = %d \n", ckksParams.LogSlots())

    btpParams := getBootstrappingParams(i)
    keygen := ckks.NewKeyGenerator(ckksParams)
    sk, pk := keygen.GenKeyPairSparse(btpParams.H)
    rlk := keygen.GenRelinearizationKey(sk, 2)
    encoder := ckks.NewEncoder(ckksParams)
    encryptor := ckks.NewEncryptor(ckksParams, pk)
    // decryptor := ckks.NewDecryptor(ckksParams, sk)

    rotations := btpParams.RotationsForBootstrapping(ckksParams.LogN(), ckksParams.LogSlots())
    rotkeys := keygen.GenRotationKeysForRotations(rotations, true, sk)
    btp, err := bootstrapping.NewBootstrapper(ckksParams, btpParams, rlwe.EvaluationKey{Rlk: rlk, Rtks: rotkeys})
    if err != nil {
        panic(err)
    }

    fmt.Println("Ciphertext generation ...")
    values := make([]complex128, ckksParams.Slots())

    for i := range values {
        values[i] = complex(1, 0)
    }
    ct := encryptor.EncryptNew(encoder.EncodeNew(values, ckksParams.LogSlots()))

    // Bootstrapping
    fmt.Println("Bootstrapping ...")
    start = time.Now()
    ct_new := btp.Bootstrapp(ct)
    fmt.Printf("Done in %s \n", time.Since(start))
    fmt.Println()

    _ = ct_new
}

func customBootstrapping() {

    // params := getCkksParams(4)
	// btpParams := getBootstrappingParams(4)
    params := getSimpleParams()
    btpParams := getCustomBootstrappingParams(params.Q(), params.Scale())

    fmt.Printf(" \n", btpParams.SlotsToCoeffsParameters.ScalingFactor)


    keygen := ckks.NewKeyGenerator(params)
    sk, pk := keygen.GenKeyPairSparse(btpParams.H)
    rlk := keygen.GenRelinearizationKey(sk, 2)
    encoder := ckks.NewEncoder(params)
    encryptor := ckks.NewEncryptor(params, pk)
    decryptor := ckks.NewDecryptor(params, sk)
    // evaluator := ckks.NewEvaluator(params, rlwe.EvaluationKey{Rlk: rlk})
    _ = pk

    printBtsLevels(btpParams)

    // Bootstrapping Keys
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
        values[i] = complex(1, 0)
    }

    ct := encryptor.EncryptNew(encoder.EncodeNew(values , params.LogSlots()))

    // Bootstrapping
    fmt.Printf("Bootstrapping ... ")
    ct_new := btp.Bootstrapp(ct)
    fmt.Printf("Done --> Level: %d, Scale: %f \n", ct_new.Level(), ct_new.Scale)

    z := encoder.Decode(decryptor.DecryptNew(ct_new), params.LogSlots())
    fmt.Printf("ct_new[0] = %f \n", real(z[0]))
}

func printBtsLevels(btpParams bootstrapping.Parameters) {
    fmt.Printf("              StartLvl Depth Depth(false) \n")
    fmt.Printf("SlotsToCoeffs:   %2d      %d      %d \n", btpParams.SlotsToCoeffsParameters.LevelStart, btpParams.SlotsToCoeffsParameters.Depth(true), btpParams.SlotsToCoeffsParameters.Depth(false))
    fmt.Printf("EvalModParams:   %2d      %d \n", btpParams.EvalModParameters.LevelStart, btpParams.EvalModParameters.Depth())
    fmt.Printf("CoeffsToSlots:   %2d      %d      %d \n", btpParams.CoeffsToSlotsParameters.LevelStart, btpParams.CoeffsToSlotsParameters.Depth(true), btpParams.CoeffsToSlotsParameters.Depth(false))
}

func getSimpleParams() (ckks.Parameters) {
    params, err := ckks.NewParametersFromLiteral(ckks.ParametersLiteral{
        LogN: 15,
        LogQ: []int{60,40,40,40,40,40,40,40,40,40},
        LogP: []int{60,60,60},
        Sigma: rlwe.DefaultSigma,
        LogSlots: 10,
        Scale: float64(1<<40),
    })
    if err != nil {
        panic(err)
    }

    return params
}

func getCustomBootstrappingParams(Q []uint64, scale float64) (bootstrapping.Parameters) {
    lenQ := len(Q)

    params := bootstrapping.Parameters{
        H: 192,
        SlotsToCoeffsParameters: advanced.EncodingMatrixLiteral{
			LinearTransformType: advanced.SlotsToCoeffs,
			LevelStart:          lenQ-7, // index
			BSGSRatio:           2.0,
			BitReversed:         false,
			ScalingFactor: [][]float64{
                {float64(Q[lenQ-8])},
                {float64(Q[lenQ-7])}, // Q[index]
			},
		},
		EvalModParameters: advanced.EvalModLiteral{
			Q:             Q[0], // Q[0]
			LevelStart:    lenQ-3,    // index
			SineType:      advanced.Cos1,
			MessageRatio:  256.0, // Q[0] / |m|
			K:             20, // interpolation [-K,K]
			SineDeg:       10,
			DoubleAngle:   0,
			ArcSineDeg:    0,
			ScalingFactor: scale, // Scale
		},
		CoeffsToSlotsParameters: advanced.EncodingMatrixLiteral{
			LinearTransformType: advanced.CoeffsToSlots,
			LevelStart:          lenQ-1, // index
			BSGSRatio:           2.0,
			BitReversed:         false,
			ScalingFactor: [][]float64{
				{float64(Q[lenQ-2])}, // Q[-2]
				{float64(Q[lenQ-1])}, // Q[-1]
			},
		},
    }

    return params
}

func printQs() {

    params := getSimpleParams()

    Qs := params.Q()
    fmt.Printf("len(Q) = %d \n", len(Qs))

    for i,q := range Qs {
        fmt.Printf("Q[%d] = %x \n", i, q)
    }
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
    fmt.Printf("Level: %2d, %2d; ct[0] = %7.2f, scale: %.1f \n", ct.Level(), ct_f.Level(), real(values[0]), math.Log2(ct.Scale))
    var z []complex128
    for ct.Level() > 0 {
        evaluator.MulRelin(ct, ct_f, ct)
        if err := evaluator.Rescale(ct, params.Scale(), ct); err != nil {
            panic(err)
        }
        z = encoder.Decode(decryptor.DecryptNew(ct), params.LogSlots())
        fmt.Printf("Level: %2d, %2d; ct[0] = %7.2f, scale: %.1f \n", ct.Level(), ct_f.Level(), real(z[0]), math.Log2(ct.Scale))
    }
}

func getCkksParamsWithSlots(paramSet int, logSlots int) (ckks.Parameters) {
    if paramSet < 0 || paramSet > 4 {
        panic("Invalid CKKS Param Set Index")
    }

    p := bootstrapping.DefaultCKKSParameters[paramSet]

    if logSlots > 0 {
        p.LogSlots = logSlots
    }

    params, err := ckks.NewParametersFromLiteral(p)
    if err != nil {
        panic(err)
    }

    return params
}

func getCkksParams(paramSet int) (ckks.Parameters) {
    return getCkksParamsWithSlots(paramSet, 0)
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
